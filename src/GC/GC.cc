#include "debug/alert.h"
#include "Object.h"
#include "GC.h"

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
}

bool GarbageCollector::add(Object* obj)
{
}

bool GarbageCollector::remove(Object* obj)
{
}
