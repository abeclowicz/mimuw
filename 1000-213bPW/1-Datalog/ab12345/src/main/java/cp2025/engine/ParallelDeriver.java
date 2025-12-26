package cp2025.engine;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicBoolean;

import cp2025.engine.Datalog.Atom;
import cp2025.engine.Datalog.Constant;
import cp2025.engine.Datalog.Predicate;
import cp2025.engine.Datalog.Program;
import cp2025.engine.Datalog.Rule;
import cp2025.engine.Datalog.Variable;

public class ParallelDeriver implements AbstractDeriver {
    private final int numWorkers;

    public ParallelDeriver(int numWorkers) {
        this.numWorkers = numWorkers;
    }

    private static Map<Predicate, List<Rule>> predicateToRules(List<Rule> rules) {
        return rules.stream().collect(
            java.util.stream.Collectors.groupingBy(rule -> rule.head().predicate()));
    }

    @Override
    public Map<Atom, Boolean> derive(Program input, AbstractOracle oracle) throws InterruptedException {
        ParallelDeriverState state = new ParallelDeriverState(input, oracle, predicateToRules(input.rules()));
        Map<Atom, Boolean> results = new HashMap<>();

        List<Callable<Task.DerivationResult>> calculations = new ArrayList<>();
        for (Atom query : input.queries())
            calculations.add(() -> {
                Task task = new Task(state);
                return task.deriveStatement(query);
            });

        // Avoid try-with-resources: when interrupted, we need to call `state.interrupt()` before
        // the executor shuts down to avoid a circular wait between tasks and `pool.close()`.
        ExecutorService pool = Executors.newFixedThreadPool(numWorkers);
        try {
            List<Future<Task.DerivationResult>> futures = pool.invokeAll(calculations);

            Iterator<Atom> it1 = input.queries().iterator();
            Iterator<Future<Task.DerivationResult>> it2 = futures.iterator();

            while (it1.hasNext() && it2.hasNext()) {
                if (Thread.interrupted())
                    throw new InterruptedException("Derivation process was interrupted.");

                results.put(it1.next(), it2.next().get().derivable);
            }
        } catch (InterruptedException | ExecutionException e) {
            state.interrupt();
            pool.shutdownNow();
            throw new InterruptedException(e.getMessage());
        } finally {
            pool.close();
        }

        return results;
    }

    private static class ParallelDeriverState {
        private final Program input;
        private final AbstractOracle oracle;
        private final Map<Predicate, List<Rule>> predicateToRules;

        private final Map<Atom, Boolean> knownStatements = new ConcurrentHashMap<>();
        private final Map<Atom, Set<Thread>> calculatingThreads = new HashMap<>();
        private final AtomicBoolean interrupted = new AtomicBoolean(false);

        public ParallelDeriverState(Program input, AbstractOracle oracle, Map<Predicate, List<Rule>> predicateToRules) {
            this.input = input;
            this.oracle = oracle;
            this.predicateToRules = predicateToRules;
        }

        public Program getInput() {
            return input;
        }

        public AbstractOracle getOracle() {
            return oracle;
        }

        public Map<Predicate, List<Rule>> getPredicateToRules() {
            return predicateToRules;
        }

        public boolean isKnownStatement(Atom statement) {
            return knownStatements.containsKey(statement);
        }

        public boolean getKnownStatement(Atom statement) {
            return knownStatements.get(statement);
        }

        private synchronized void takeCalculatableAtom(Atom goal) {
            // Clear current thread's interrupted status.
            Thread.interrupted();
            // This ensures that if the thread was interrupted earlier
            // (e.g. during a previous derivation, but too late to catch InterruptedException)
            // it starts this calculation with a clean state.

            calculatingThreads
                .computeIfAbsent(goal, p -> new HashSet<>())
                .add(Thread.currentThread());
        }

        private synchronized void dropCalculatableAtom(Atom goal) {
            calculatingThreads.get(goal).remove(Thread.currentThread());
        }

        private synchronized void addKnownStatement(Atom statement, boolean result) {
            knownStatements.put(statement, result);

            // Interrupt threads calculating the same statement,
            // so they can use the result from this thread.
            if (calculatingThreads.containsKey(statement))
                calculatingThreads.get(statement).forEach(Thread::interrupt);
        }

        public void interrupt() {
            interrupted.set(true);
        }

        public boolean isInterrupted() {
            return interrupted.get();
        }
    }

    private static class Task {
        private final ParallelDeriverState state;

        private final Set<Atom> inProgressStatements = new HashSet<>();

        public Task(ParallelDeriverState state) {
            this.state = state;
        }

        private record DerivationResult(boolean derivable, Set<Atom> failedStatements) {}

        /**
         * @throws InterruptedException if the main thread executing
         * {@link AbstractDeriver#derive(Program, AbstractOracle) derive(program, oracle)}
         * is interrupted.
         */
        public DerivationResult deriveStatement(Atom goal) throws InterruptedException {
            // Check if we already know the result for this statement.
            if (state.isKnownStatement(goal))
                return new DerivationResult(state.getKnownStatement(goal), Set.of());

            // Check if the main thread executing `derive(program, oracle)` is interrupted.
            if (state.isInterrupted())
                throw new InterruptedException("Derivation process was interrupted.");

            // Check if the statement is calculatable.
            if (state.getOracle().isCalculatable(goal.predicate())) {
                // Mark that the current thread is calculating this goal.
                state.takeCalculatableAtom(goal);

                try {
                    boolean result = state.getOracle().calculate(goal);
                    state.addKnownStatement(goal, result);
                    return new DerivationResult(result, Set.of());
                } catch (InterruptedException e) {
                    // Check if the main thread executing `derive(program, oracle)` is interrupted.
                    if (state.isInterrupted())
                        throw e;

                    // Otherwise, the interruption came from another thread,
                    // so we already know the result for this statement.
                    return new DerivationResult(state.getKnownStatement(goal), Set.of());
                } finally {
                    // Indicate that the current thread has finished calculating this goal.
                    state.dropCalculatableAtom(goal);
                }
            }

            // Check for cycles, to avoid infinite loops.
            if (inProgressStatements.contains(goal))
                // Return false but do not store the result (we may find a different derivation later).
                return new DerivationResult(false, Set.of(goal));

            // Try to actually derive the statement using rules.
            inProgressStatements.add(goal);
            try {
                DerivationResult result = deriveNewStatement(goal);

                if (result.derivable)
                    state.addKnownStatement(goal, true);
                // We can only deduce non-derivability when there are no in-progress statements
                // (at the top of the recursion, excluding the current statement itself).
                else if (inProgressStatements.size() == 1)
                    for (Atom statement : result.failedStatements) {
                        // Check if the main thread executing `derive(program, oracle)` is interrupted.
                        if (state.isInterrupted())
                            throw new InterruptedException("Derivation process was interrupted.");

                        state.addKnownStatement(statement, false);
                    }

                return result;
            } finally {
                inProgressStatements.remove(goal);
            }
        }

        /**
         * @throws InterruptedException if the main thread executing
         * {@link AbstractDeriver#derive(Program, AbstractOracle) derive(program, oracle)}
         * is interrupted.
         */
        private DerivationResult deriveNewStatement(Atom goal) throws InterruptedException {
            // We assume that everything that follows is deterministic.
            //
            // This guarantees that we handle the situation described below correctly:
            //   > Należy również przerwać wyprowadzanie tych stwierdzeń, dla których
            //   > ustalenie wyprowadzalności nie jest już potrzebne
            //   > (bo były wyprowadzane tylko w celu wyprowadzenia s).
            //
            // In the following scenario:
            //   - Thread A has already derived Atom X, but
            //   - Thread B is still deriving some child of Atom X
            // determinism ensures that B cannot go any lower in the dependency tree,
            // as it would re-visit an already processed Atom.
            //
            // Therefore, when B attempts to descend into the next, lower-level child,
            // it will immediately see that the Atom has already been derived and will
            // stop the derivation, unwinding the call stack back up.

            List<Rule> rules = state.getPredicateToRules().get(goal.predicate());
            if (rules == null)
                return new DerivationResult(false, Set.of(goal));

            Set<Atom> failedStatements = new HashSet<>();

            for (Rule rule : rules) {
                // Check if we already know the result for this statement.
                if (state.isKnownStatement(goal))
                    return new DerivationResult(state.getKnownStatement(goal), Set.of());

                // Check if the main thread executing `derive(program, oracle)` is interrupted.
                if (state.isInterrupted())
                    throw new InterruptedException("Derivation process was interrupted.");

                Optional<List<Atom>> partiallyAssignedBody = Unifier.unify(rule, goal);
                if (partiallyAssignedBody.isEmpty())
                    continue;

                List<Variable> variables = Datalog.getVariables(partiallyAssignedBody.get());
                FunctionGenerator<Variable, Constant> iterator = new FunctionGenerator<>(variables, state.getInput().constants());
                for (Map<Variable, Constant> assignment : iterator) {
                    // Check if we already know the result for this statement.
                    if (state.isKnownStatement(goal))
                        return new DerivationResult(state.getKnownStatement(goal), Set.of());

                    // Check if the main thread executing `derive(program, oracle)` is interrupted.
                    if (state.isInterrupted())
                        throw new InterruptedException("Derivation process was interrupted.");

                    List<Atom> assignedBody = Unifier.applyAssignment(partiallyAssignedBody.get(), assignment);
                    DerivationResult result = deriveBody(goal, assignedBody);
                    if (result.derivable)
                        return new DerivationResult(true, Set.of());
                    failedStatements.addAll(result.failedStatements);
                }
            }

            failedStatements.add(goal);
            return new DerivationResult(false, failedStatements);
        }

        /**
         * @throws InterruptedException if the main thread executing
         * {@link AbstractDeriver#derive(Program, AbstractOracle) derive(program, oracle)}
         * is interrupted.
         */
        private DerivationResult deriveBody(Atom goal, List<Atom> body) throws InterruptedException {
            for (Atom statement : body) {
                // Check if we already know the result for this statement.
                if (state.isKnownStatement(goal))
                    return new DerivationResult(state.getKnownStatement(goal), Set.of());

                // Return failure as soon as any statement in the body fails.
                DerivationResult result = deriveStatement(statement);
                if (!result.derivable)
                    return new DerivationResult(false, result.failedStatements);
            }

            return new DerivationResult(true, Set.of());
        }
    }
}
