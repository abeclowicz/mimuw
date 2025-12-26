#include "named_poset_collections.h"

#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include <stdbool.h>
#include <string.h>

static bool streq(char const *s1, char const *s2) {
  if (s1 == s2)
    return true;

  if (s1 == NULL || s2 == NULL)
    return false;

  if (strcmp(s1, s2) == 0)
    return true;

  return false;
}

static char const *names[2] = {"abcd", "xyz"};

int main() {
  long id1;
  id1 = npc_new_collection();
  assert(id1 == 0);
  assert(npc_size() == 1);
  assert(npc_collection_size(id1) == 0);
  assert(npc_new_poset(id1, names[1]));
  assert(npc_new_poset(id1, names[0]));
  assert(!npc_new_poset(id1, names[1]));
  assert(npc_collection_size(id1) == 2);
  int i = 0;
  for (char const *name = npc_first_poset(id1); name != NULL; name = npc_next_poset(id1, name))
    assert(streq(name, names[i++]));
  assert(i == 2);
  npc_delete_poset(id1, names[1]);
  npc_delete_poset(id1, names[1]);
  assert(npc_collection_size(id1) == 1);
  i = 0;
  for (char const *name = npc_first_poset(id1); name != NULL; name = npc_next_poset(id1, name))
    assert(streq(name, names[i++]));
  assert(i == 1);
  assert(npc_is_relation(id1, names[0], 0, 0));
  assert(!npc_is_relation(id1, names[0], 0, 1));
  assert(npc_add_relation(id1, names[0], 0, 1));
  assert(npc_add_relation(id1, names[0], 1, 2));
  assert(npc_is_relation(id1, names[0], 0, 2));
  assert(npc_remove_relation(id1, names[0], 0, 1));
  assert(npc_is_relation(id1, names[0], 0, 2));
  assert(npc_is_relation(id1, names[0], 1, 2));
  npc_delete_collection(id1);
  assert(npc_size() == 0);
  assert(npc_poset_size() == 32);
}
