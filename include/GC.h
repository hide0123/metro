#pragma once

// ---------------------------------------------
//  Garbage Collector
// ---------------------------------------------

struct Object;
class GarbageCollector {
public:

  GarbageCollector();
  ~GarbageCollector();

  void register_object(Object* obj);

  void clean();

  void execute();


private:

  std::vector<Object*> objects;

};
