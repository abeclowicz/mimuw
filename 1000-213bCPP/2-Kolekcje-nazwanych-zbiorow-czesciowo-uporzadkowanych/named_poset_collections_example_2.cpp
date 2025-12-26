#include "named_poset_collections.h"

#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <cassert>

namespace {
  long test() {
    long id = ::cxx::npc_new_collection();
    ::cxx::npc_new_poset(id, "poset");
    return id;
  }

  long id = test();
}

int main() {
  assert(id == 0);
  assert(::cxx::npc_size() == 1);
  assert(::cxx::npc_collection_size(id) == 1);
  ::cxx::npc_delete_collection(id);
  assert(::cxx::npc_size() == 0);
  assert(::cxx::npc_collection_size(id) == 0);
  assert(::cxx::npc_poset_size() == 64);
}
