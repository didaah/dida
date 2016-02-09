<?php
//$Id$

/**
 * 删除缓存
 * @param (string) $type
 *  删除模式：
 *   cid => 删除指定键名
 *   * => 通配符模式
 *   all => 清空全部缓存表
 *   empty => 清空指定表
 *   table => 删除指定表除永久缓存外的所有数据
 * @param (string) $cid
 *  缓存键名
 * @param (string) $table
 *  表名
 */
function cache_del($type = 'cid', $cid = NULL, $table = 'cache') {
  switch ($type) {
    case 'cid':
      db_exec('DELETE FROM {'. $table .'} WHERE cid = ?', array($cid));
    break;
    case 'all':
      db_exec('TRUNCATE TABLE {cache}');
      db_exec('TRUNCATE TABLE {cache_menu}');
      db_exec('TRUNCATE TABLE {cache_page}');
      db_exec('TRUNCATE TABLE {cache_form}');
    break;
    case 'empty':
      db_exec('TRUNCATE TABLE {'. $table .'}');
    break;
    case '*':
      db_exec('DELETE FROM {'. $table .'} WHERE cid LIKE ?', array('%'.$cid.'%'));
    break;
    case 'table':
      db_exec('DELETE FROM {'. $table .'} WHERE expire = 0');
  }
}

/**
 * 获取缓存
 * @param (string) $cid
 *  缓存键名
 * @param (string) $table
 *  表名
 * @reutrn object or 0
 */
function cache_get($cid, $table = 'cache') {
  $cache = db_query('SELECT data, created, expire, serialized FROM {'. $table .'} WHERE cid = ?', array($cid), array('return' => 'one'));
  if (isset($cache->created)) {
    if (!$cache->expire || $cache->expire > $_SERVER['REQUEST_TIME']) {
      if ($cache->serialized) {
        $cache->data = unserialize($cache->data);
      }
      return $cache;
    }
  }
  return 0;
}

/**
 * 写入缓存
 * @param (string) $cid
 *  缓存键名，推荐以模块名称开头，如：system_data_cache
 * @param (*) $data
 *  需缓存的数据，任意类型
 * @param (string) $table
 *  存储的表名
 * @param (int) $expire
 *  过期时间，以当前日期为基础，秒为单位。如：500，表示 500 秒后过期。默认为 0，即永不过期
 * @param (bool) $replace
 *  是否使用 db_replace 写入数据
 */
function cache_set($cid, $data, $table = 'cache', $expire = NULL, $replace = 1) {
  $cache = array();
  
  $cache['serialized'] = 0; // 序列化
  
  if (is_object($data) || is_array($data)) {
    $data = serialize($data);
    $cache['serialized'] = 1;
  }
  
  $cache['data'] = $data;
  
  if ($expire) {
    $cache['expire'] = $expire+$_SERVER['REQUEST_TIME'];
  } else {
    $cache['expire'] = 0;
  }
  
  $cache['cid'] = $cid;
  $cache['created'] = $_SERVER['REQUEST_TIME'];
  
  if ($replace) {
    return db_replace($table, $cache);
  } else {
    db_exec('DELETE FROM {'.$table.'} WHERE cid = ?', array($cache['cid']));
    return db_exec('INSERT INTO {'.$table.'} (cid, data, expire, created, serialized) VALUES (?, ?, ?, ?, ?)',
    array($cache['cid'], $cache['data'], $cache['expire'], $cache['created'], $cache['serialized']));
  }
}
