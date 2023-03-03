#include "Object.h"
#include "GC.h"
#include "debug/alert.h"

#define GC_

static std::vector<Object*>* objects;

void GarbageCollector::execute()
{
}

void GarbageCollector::final()
{
}

void GarbageCollector::set_object_list(std::vector<Object*>* vec)
{
  objects = vec;
}

void GarbageCollector::clean()
{
  auto& e = *objects;

  for (auto&& obj : e) {
    if (!obj || obj->no_delete)
      continue;

    if (obj->ref_count == 0) {
      alertmsg("delete obj: " << obj->to_string());
      delete obj;
      obj = nullptr;
    }
  }
}

bool GarbageCollector::add(Object* obj)
{
  auto& e = *objects;

  for (auto&& x : e) {
    if (x == obj) {
      return false;
    }
    else if (x == nullptr) {
      x = obj;
      return true;
    }
  }

  objects->emplace_back(obj);
  return true;
}

bool GarbageCollector::remove(Object* obj)
{
  for (auto it = objects->begin(); it != objects->end(); it++) {
    if (*it == obj) {
      objects->erase(it);
      return 1;
    }
  }

  return false;
}
