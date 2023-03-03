#pragma once

#include <vector>

// ---------------------------------------------
//  Garbage Collector
// ---------------------------------------------

struct Object;
class GarbageCollector {
public:
  /**
   * @brief 起動
   */
  static void execute();

  /**
   * @brief 終了
   */
  static void final();

  /**
   * @brief
   *
   */
  static void set_object_list(std::vector<Object*>* vec);

  /**
   * @brief 使用されていないオブジェクトを削除する
   */
  static void clean();

  /**
   * @brief オブジェクトを追加
   *
   * @param obj
   * @return すでに追加されている場合は、false
   */
  static bool add(Object* obj);

  /**
   * @brief オブジェクトを削除
   *
   * @param obj
   * @return 追加されていない場合は、false
   */
  static bool remove(Object* obj);

  /**
   * @brief 全てのオブジェクトを取得する
   *
   * @return std::vector<Object*> const&
   */
  static std::vector<Object*> const& get_objects();
};
