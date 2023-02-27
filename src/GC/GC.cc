#include "Object.h"
#include "GC.h"

GarbageCollector::GarbageCollector() {
  
}

GarbageCollector::~GarbageCollector() {
  
}

void GarbageCollector::register_object(Object* obj) {
  this->objects.emplace_back(obj);
}

void GarbageCollector::clean() {

}

