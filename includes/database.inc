<?php
// $Id$

/**
 * 数据库信息，扩展文件必须实现此接口
 */
function db_info() {
  $types = array();
  $hd = dir('includes/database');
  while (false !== ($name = $hd->read())) {
    if (strpos($name, 'database') !== false) {
      if ($a = explode('.', $name)) {
        require_once DIDA_ROOT . '/includes/database/database.'.$a[1].'.inc';
        $function = $a[1].'_db_info';
        if (function_exists($function)) {
          $types += $function();
        }
      }
    }
  }
  return $types;
}

/**
 * 数据库连接
 * @param (string) $target
 *  连接参数，默认为 $database['default'] 中提供的信息
 * @return (bool)
 *  根据连接参数调用相应的扩展文件，如：database.mysql.inc
 *  连接成功，返回true，并将连接对象写入全局变量 $db。失败则返回 false
 */
function db_connect($target = 'default') {
  global $database, $dbc, $db, $_db;

  if (!isset($db[$target]) && $dbc[$target]  = $database[$target]) {
    $function = 'db_'.$dbc[$target]['driver'].'_connect';
    $dbc[$target]['target'] = $target;
    require_once DIDA_ROOT . '/includes/database/database.'.$dbc[$target]['driver'].'.inc';
    if (function_exists($function)) {
      if ($db[$target] = $function($dbc[$target], $error)) {
      	return true;
      } else {
      	dd_set_message(t('system', '数据库无法连接：@text', array('@text' => $error)), 'debug');
      }
    }
  }
}

/**
 * select 推荐使用
 * @param (string) $sql：语句，参数使用占位符
 * @param (array) $args：参数，对应语句中的占位符
 *
 * @param (array) $opt：属性设置
 *  $opt['target']：选择数据库连接
 *  $opt['fetch']：返回类型
 *   object - PDO::FETCH_OBJ，默认
 *   array - PDO::FETCH_ASSOC
 *   或其它 PDOStatement::setFetchMode() 允许的参数
 *
 *  $opt['return']：返回结果
 *    object - PDOStatement->fetchAll()，默认
 *    array - PDOStatement->fetch()
 *    column - PDOStatement->fetchColumn()
 *    result - PDOStatement
 *
 *  $opt['limit']：查询起始，array(0, 10) or 10
 *
 * @return object or array or string
 *
 */
function db_query($sql, $args = array(), $opt = array()) {
  $opt = db_default_options($opt);
  $sql = db_prefix_tables($sql, $opt);
  $function = 'db_' . $opt['driver'].'_query';
  
  if (!empty($GLOBALS['conf']['sql_info'])) {
    $start = microtime(true);
  }
 
  if (!empty($args)) db_args_filter($sql, $args);

  $fetch = $function($sql, $args, $opt);
  
  if (!empty($GLOBALS['conf']['sql_info'])) {
    db_debug(microtime(true) - $start);
  }
  
  return $fetch;
}

/**
 * 将参数中的数组分拆解析 
 * @param string $sql 
 * @param array $args 
 * @access public
 * @return void
 */
function db_args_filter(&$sql, &$args) {
  // 如果占位符对应的参数是一个数组，分拆，当成 IN 传入
  foreach (array_filter($args, 'is_array') as $key => $data) {
    
    // 特殊占位符，不做解析，参见 database.mysql.inc db_mysql_exec
    if ($key == 'values' || $key == 'placeholder') continue;

    $new_keys = array();

    foreach ($data as $i => $value) {
      $new_keys[$key . '_' . $i] = $value;
    }

    $sql = preg_replace('#' . $key . '\b#', implode(', ', array_keys($new_keys)), $sql);
    
    unset($args[$key]);

    $args += $new_keys;
  }
}

/**
 * insert、update、delete 推荐使用
 * @param (string) $sql：语句，参数使用占位符
 * @param (array) $args：参数，对应语句中的占位符
 * @param (array) $opt：属性设置
 *
 * @return (int) $count 影响的列数
 *
 */
function db_exec($sql, $args = array(), array $opt = array()) {
  $opt = db_default_options($opt);
  
  if ($sql) $sql = db_prefix_tables($sql, $opt);
  $function = 'db_' . $opt['driver'].'_exec';
  
  if (!empty($GLOBALS['conf']['sql_info'])) {
    $start = microtime(true);
  }
  
  if (!empty($args))  db_args_filter($sql, $args);

  $fetch = $function($sql, $args, $opt);
  
  if (!empty($GLOBALS['conf']['sql_info'])) {
    db_debug(microtime(true) - $start);
  }
  return $fetch;
}

/**
 * 插入或替换记录，对应 mysql REPLACE INTO 用法，其它数据库类型，根据自身特性实现
 * @param (string)$table
 *  数据表名
 * @param (object or array) $args
 *  待写入的数据，可以是 object 或 array。其单元对应字段名称
 * @param (array) $opt
 *   数据库属性设置
 * @return
 *  影响的列数
 */
function db_replace($table, $args, array $opt = array()) {
  $opt = db_default_options($opt);
  
  $function = 'db_' . $opt['driver'].'_replace';
  
  if (!empty($GLOBALS['conf']['sql_info'])) {
    $start = microtime(true);
  }
  
  $fetch = $function($table, $args, $opt);
  
  if (!empty($GLOBALS['conf']['sql_info'])) {
    db_debug(microtime(true) - $start);
  }
  return $fetch;
}

/**
 * 检查表是否存在
 * @param string $table
 *  表名
 * @param string $target
 *  连接名称
 * @global $dbc
 * @access public
 * @return bool
 */
function db_is_table($table, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);
  
  //$table = $dbc[$target]['prefix'] . $table;
  
  return call_user_func('db_' . $dbc[$target]['driver'].'_is_table', $table, $target);
}

/**
 * 检查字段是否存在
 */
function db_is_field($table, $field, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);
  
  return call_user_func('db_' . $dbc[$target]['driver'].'_is_field', $table, $field, $target);
}

/**
 * 列出所有表
 */
function db_show_tables($target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  
  return call_user_func('db_' . $dbc[$target]['driver'].'_show_tables', $target);
}

/**
 * 获取数据库版本
 */
function db_get_version($target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  
  return call_user_func('db_' . $dbc[$target]['driver'].'_get_version', $target);
}

/**
 * 检查属性中是否有链接信息
 */
function db_default_options(array $opt = array()) {
  global $dbc;
  if (empty($opt['target'])) $opt['target'] = 'default';
  if (empty($dbc[$opt['target']])) db_connect($opt['target']);
  $opt['driver'] = $dbc[$opt['target']]['driver'];
  return $opt;
}

/**
 * 为查询语句中的表名加上前缀
 */
function db_prefix_tables($sql, $opt) {
  global $dbc;
  $prefix = $dbc[$opt['target']]['prefix'];
  return strtr($sql, array('{' => $prefix, '}' => ''));
}

/**
 * 获取最后一个自增主键
 */
function db_last_insert_id($target = 'default') {
  return $GLOBALS['db'][$target]->lastInsertId();
}

/**
 * @param $code：错误代码
 * @param $info：错误描述
 */
function db_error($code, $info) {
  if ($code != '0000') {
    $msg = 'Error('.$info[0].')：' . dd_array_to_string($info);
   
    if ($debug = debug_backtrace()) {
      $item = array();
      foreach (array_reverse($debug) as $i => $de) {
        if (strpos($de['file'], 'index.php') === false && $de['function'] != 'db_error') {
          if ($de['file']) {
            $items[] = '<li>'.t('system', '文件：@file(@line 行，函数 @fun)',
            array('@file' => $de['file'], '@line' => $de['line'], '@fun' => $de['function'])).'</li>';
          }
        }
        
        if ($de['function'] == 'db_query' || $de['function'] == 'db_exec') {
          $items[] = $de['function'] . ':' . $de['args'][0];
          $items[] = dd_array_to_string($de['args'][1]);
          $items[] = dd_array_to_string($de['args'][2]);
        }
      }
      $msg .= implode("\n", $items);
    }
    
    text_log('database_error', dd_error_msg($msg));

    if (!$GLOBALS['conf']['debug']) return;
    if ($GLOBALS['conf']['debug'] == 2 && $GLOBALS['user']->uid != 1) return;
    
    dd_set_message($msg, 'error');
    return true;
  }
}


/**
 * 创建表
 * @param string $name
 *  数据表名称
 * @param string $table
 *  字段名称
 * @return string|false
 */
function db_create_table($name, $table, $target = 'default') {
  global $dbc, $db;
  
  if (empty($dbc[$target])) db_connect($target);
  
  if (!db_is_table($name, $target)) {
    if ($sql = call_user_func('db_' . $dbc[$target]['driver'].'_create_table', $name, $table, $target)) {
      return $sql;
    } else {
      dd_set_message(t('system', '表 @table 创建失败', array('@table' => $name)), 'error');
    }
  } else {
    dd_set_message(t('system', '表 @table 已经存在', array('@table' => $name)), 'error');
  }
}

/**
 * 表更名
 * @param (string)$name
 *  原表名
 * @param (string)$new_name
 *  新表名
 * @param (string)$target
 *  数据库
 */
function db_rename_table($name, $new_name, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  
	if (db_is_table($name, $target)) {
		if (!db_is_table($new_name, $target)) {
	    if (call_user_func('db_' . $dbc[$target]['driver'].'_rename_table', $name, $new_name, $target)) {
	      dd_set_message(t('system', '@table 更名成功', array('@table' => $name)));
	      return true;
	    } else {
	      dd_set_message(t('system', '@table 更名失败', array('@table' => $name)), 'error');
	    }
		} else {
			dd_set_message(t('system', '@table 已经存在', array('@table' => $new_name)), 'error');
		}
	} else {
		dd_set_message(t('system', '@table 不存在', array('@table' => $name)), 'error');
	}
}

/**
 * 删除数据表 
 * @param string $table 
 *  表名
 * @param string $target 
 *  连接名称
 * @global $dbc
 * @access public
 * @return bool
 */
function db_drop_table($table, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);
  if (db_is_table($table, $target)) {
    if (call_user_func('db_' . $dbc[$target]['driver'].'_drop_table', $table, $target)) {
      dd_set_message(t('system', '@table 删除成功', array('@table' => $table)));
      return true;
    } else {
      dd_set_message(t('system', '@table 删除失败', array('@table' => $table)), 'error');
    }
  } else {
    dd_set_message(t('system', '@table 不存在', array('@table' => $table)), 'error');
  }
}

/**
 * 添加字段
 */
function db_add_field($table, $field, $spec, $keys_new = array(), $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_add_field', $table, $field, $spec, $keys_new, $target);
}

/**
 * 删除字段
 */
function db_drop_field($table, $field, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_drop_field', $table, $field, $target);
}

/**
 * 设置字段默认值
 * @param $table
 *  表名
 * @param $field
 *  字段名
 * @param $value
 *  默认值
 */
function db_field_set_default($table, $field, $value, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_field_set_default', $table, $field, $value, $target);
}

/**
 * 删除字段默认值
 */
function db_field_drop_default($table, $field, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_field_del_default', $table, $field, $target);
}

/**
 * 添加主键
 */
function db_add_primary($table, $fields, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_add_primary', $table, $fields, $target);
}

/**
 * 删除主键
 */
function db_drop_primary($table, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_drop_primary', $table, $target);
}

/**
 * 添加外键
 */
function db_add_foreign($table, $field, $foreign, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_add_foreign', $table, $field, $foreign, $target);
}

/**
 * 删除外键
 */
function db_drop_foreign($table, $field, $parent_table, $parent_field, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_drop_foreign', $table, $field, $parent_table, $parent_field, $target);
}

/**
 * 添加唯一值
 */
function db_add_unique($table, $name, $fields, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_add_unique', $table, $name, $fields, $target);
}

/**
 * 删除唯一值
 */
function db_drop_unique($table, $name, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_drop_unique', $table, $name, $target);
}

/**
 * 添加索引
 */
function db_add_index($table, $name, $fields, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_add_index', $table, $name, $fields, $target);
}

/**
 * 删除索引
 */
function db_drop_index($table, $name, $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_drop_index', $table, $name, $target);
}

/**
 * 更新字段
 */
function db_change_field($table, $field, $field_new, $spec, $keys_new = array(), $target = 'default') {
  global $dbc;
  if (empty($dbc[$target])) db_connect($target);
  return call_user_func('db_' . $dbc[$target]['driver'].'_change_field', $table, $field, $field_new, $spec, $keys_new, $target);
}

/**
 * 数组解析为占位符
 */
function db_get_placeholder($array = array()) {
	return implode(',', array_fill(0, count($array), '?'));
}

/**
 * 快捷的数据库 insert、update 操作接口，适合较多字段的表写入、更新操作，可为模块开发者减少 sql 代码量
 * @param (string)$table
 *  数据表名
 * @param (object or array) &$o
 *  待写入的数据，可以是 object 或 array。其单元对应字段名称。若成功，将附加一个 _db_write_record 单元
 * @param (array) $wheres
 *  如果是更新操作，应当传递做为条件的字段名称，字段之间的关系只能是 AND
 * @param (array) $opt
 *   db_exec() 接口 $opt 参数
 * @param string|array $table_field
 *  指定待操作的表字段(array)，或表名(string)，这将跳过检查表是否存，通常用在向其它库插入内容
 * @return
 *  成功返回 true，update 时影响的列数，将赋给 _db_write_record 单元，失败返回 false。
 */
function db_write_record($table, &$o, $wheres = array(), array $opt = array(), $table_field = NULL) {
  if (!empty($table_field)) {
    if (is_array($table_field)) {
      $tables = $table_field;
    } else {
      $tables = module_get_schema($table_field);
    }
  } else {
    $tables = module_get_schema($table);
  }
  
  if (!is_array($tables['fields'])) {
    if (db_is_table($table)) {
      $tables = module_get_schema($table, 1);
    } else {
      dd_set_message(t('system', '无效的数据表 !string', array('!string' => $table)), 'error');
      return false;
    }
  }
  
  if (is_object($o)) {
    $o = (array) $o;
    $toObject = true;
  } else {
    $toObject = false;
  }
  
  if (!is_array($o)) {
    dd_set_message(t('system', '待写入内容必须是对象或数组'), 'error');
    return false;
  }
  
  $args = array();
  
  // 自增字段标记
  $insert_id = NULL;
  
  // 操作状态，$wheres 有值，则意味着 update，反之则为 insert
  $o['_db_write_record_is_update'] = count($wheres);
  
  foreach ($tables['fields'] as $field => $info) {
    // 自增字段且处于 insert 状态，跳过
    if ($info['type'] == 'serial' && !$o['_db_write_record_is_update']) {
      $insert_id = $field;
      continue;
    }
    
    if (isset($o[$field])) {
      // 字段与值均存在
      if (!$info['serialize']) {
        $args[$field] = $o[$field];
      } else {
        $args[$field] = serialize($o[$field]);
      }
      
    } else if (!$o['_db_write_record_is_update']) {
      /**
       * 字段存在，无值，且处于 insert 状态，才试着为字段生成默认值
       */
      
      // 若字段设置了默认值，不做处理，无默认值，根据类型设置默认值
      if (!isset($info['default'])) {
        
        // 自增字段，添加标记
        if ($info['type'] == 'serial') {
          $insert_id = $field;
          continue;
        }
        
        // 不允许 null
        if ($info['not null']) {
          
          if ($info['type'] == 'int' || $info['type'] == 'float' || $info['type'] == 'numeric') {
            // 数字类型，默认值设为 0
            $args[$field] = 0;
          } else {
            // 文本或其它
            $args[$field] = '';
          } 
        } else {
          // 允许 null
          $args[$field] = NULL;
        }
      }
    }
    
  }
  
  /**
   * 更新操作，获取执行条件
   * $wheres 是一个由执行条件的字段名称构成的数组
   */
  if ($o['_db_write_record_is_update']) {
    
    foreach ($args as $field_name => $value) {
      $placeholder = ':'.$field_name;
      
      if (in_array($field_name, $wheres)) {
        $where[] = $field_name .' = '.$placeholder;
      } else {
        $sets[] = $field_name .' = '.$placeholder;
      }
      
      $sql_args[$placeholder] = $value;
    }
    
    if ($where) {
      $sql = 'UPDATE {'.$table.'} SET '. implode(',', $sets) . ' WHERE '.implode(' AND ', $where);
      
      if (!isset($opt['return'])) {
        $opt['return'] = true;
      }
      if ($count = db_exec($sql, $sql_args, $opt)) {
        if ($count === true) {
          $o['_db_write_record'] = 0;
        } else {
          $o['_db_write_record'] = $count;
        }
        
      } else {
        dd_set_message(t('system', '更新失败'), 'error');
        return false;
      }
    } else {
      dd_set_message(t('system', '更新条件不正确'), 'error');
      return false;
    }
    
  } else {
    
    foreach ($args as $field_name => $value) {
      $placeholder = ':'.$field_name;
      $fields[] = $field_name;
      $placeholders[] = $placeholder;
      $sql_args[$placeholder] = $value;
    }
    
    $sql = 'INSERT INTO {'.$table.'} ';
    $sql .= '('.implode(',', $fields).') VALUES ';
    $sql .= '('.implode(',', $placeholders).')';
    
    if (db_exec($sql, $sql_args, $opt)) {
      
      if ($insert_id) {
        $o[$insert_id] = db_last_insert_id();
      }
      
    } else {
      dd_set_message(t('system', '写入失败'), 'error');
      return false;
    }
  }
  
  if ($toObject) {
    $o = (object) $o;
  }
  
  return true;
}

/**
 * 在页脚显示查询语句
 */
function db_debug($time) {
  if (!$GLOBALS['conf']['sql_info']) return;
  if ($GLOBALS['conf']['sql_info'] == 2 && $GLOBALS['user']->uid != 1) return;
  
  static $items, $repeats;
  
  if ($debug = debug_backtrace()) {
    $items[] = '<li>耗时：'.$time.'</li>';
    foreach (array_reverse($debug) as $i => $de) {
      if (strpos($de['file'], 'index.php') === false && $de['function'] != 'db_debug') {
        if ($de['file']) {
          $items[] = '<li>'.t('system', '文件：@file(@line 行，函数 @fun)',
          array('@file' => $de['file'], '@line' => $de['line'], '@fun' => $de['function'])).'</li>';
        }
      }
      
      if ($de['function'] == 'db_query' || $de['function'] == 'db_exec') {
        
        $args_string = dd_array_to_string($de['args'][1]);
        $keys = md5($de['args'][0].$args_string);
        
        if (!isset($repeats[$keys])) {
          $repeats[$keys] = array('sql' => $de['args'][0].'<br />args：'.$args_string, 'count' => 1);
        } else {
          $repeats[$keys]['count'] += 1;
        }
        
        $rows[] = array('data' => $de['args'][0], 'class' => 'dd_database_debug_args0');
        
        if ($de['args'][1]) {
          $rows[] = array('data' => $args_string, 'class' => 'dd_database_debug_args1');
        } else {
          $rows[] = '';
        }
        
        if ($de['args'][2]) {
          $rows[] = array('data' => dd_array_to_string($de['args'][2]), 'class' => 'dd_database_debug_args2');
        } else {
          $rows[] = '';
        }
        
        $_SESSION['database_debug'][] = $rows;
        
        $_SESSION['database_debug'][][] = array(
          'data' => ($items ? '<ul>'.implode('', $items).'</ul>' : ''),
          'colspan' => 3
        );
        
        $items = array();
      }
    }
    
    if ($repeats) {
      foreach ($repeats as $key => $sql) {
        if ($sql['count'] > 1) {
          $_SESSION['database_debug_count'][$key][0] = t('system', '!sql 执行了 %count 次',
            array('!sql' => $sql['sql'], '%count' => $sql['count']));
        }
      }
    }
  }
}

/**
 * 数据库与数据库用户的相关操作，可能需要连接贴拥有较高权限，如创建数据库、删除数据库等
 * 若实现这些接口，在安装时可提供更好的用户体验，非必须实现，若无法实现，可为接口返回 false，如：
 *  function mysql_db_drop_database($database, $target) {
 *    return false;
 *  }
 */

/**
 * 检查数据库是否存在
 * @param string $database 
 *  数据库名称
 * @param string $target 
 *  连接名称
 * @global $dbc
 * @access public
 * @return bool
 */
function db_is_database($database, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);
  
  return call_user_func('db_' . $dbc[$target]['driver'].'_is_database', $database, $target);
}

/**
 * 创建一个数据库 
 * @param string $database 
 *  数据库名称
 * @param string $target 
 *  连接名称
 * @global $dbc
 * @access public
 * @return bool
 */
function db_create_database($database, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);

  if (!db_is_database($database, $target)) {
    if (call_user_func('db_' . $dbc[$target]['driver'].'_create_database', $database, $target)) {
      dd_set_message(t('system', '@database 创建成功', array('@database' => $database)));
      return true;
    } else {
      dd_set_message(t('system', '@database 创建失败', array('@database' => $database)), 'error');
    }
  } else {
    dd_set_message(t('system', '@database 已存在', array('@database' => $database)), 'error');
  }
}

/**
 * 删除一个数据库
 * @param string $database
 *  表名
 * @param string $target 
 *  连接名称
 * @global $dbc
 * @access public
 * @return bool
 */
function db_drop_database($database, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);

  if (db_is_database($database, $target)) {
    if (call_user_func('db_' . $dbc[$target]['driver'].'_drop_database', $database, $target)) {
      dd_set_message(t('system', '@database 删除成功', array('@database' => $database)));
      return true;
    } else {
      dd_set_message(t('system', '@database 删除失败', array('@database' => $database)), 'error');
    }
  } else {
    dd_set_message(t('system', '@database 不存在', array('@database' => $database)), 'error');
  }
}

/**
 * 检查数据库用户是否存在 
 * @param string $username 
 * @param string $target 
 * @access public
 * @return bool
 */
function db_is_user($username, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);
  
  return call_user_func('db_' . $dbc[$target]['driver'].'_is_user', $username, $target);
}

/**
 * 创建一个数据库用户，并分配一个数据库全部权限 
 * @param string $username
 *  用户名
 * @param string $password
 *  密码
 * @param string $target
 *  连接名称
 * @param  string $database
 *  赋予指定数据库的全部权限
 * @param string $host
 *  允许的来源主机，默认为所有主机
 * @access public
 * @return bool
 */
function db_create_user($username, $password, $database, $host = '%', $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);

  if (db_is_user($username, $target)) {
    dd_set_message(t('system', '数据库用户 @username 已经存在', array('@username' => $username)), 'error');
  } else if (!db_is_database($database, $target)) {
    dd_set_message(t('system', '数据库 @database 不存在', array('@database' => $database)), 'error');
  } else if (call_user_func('db_' . $dbc[$target]['driver'].'_create_user', $username, $password, $database, $host, $target)) {
    dd_set_message(t('system', '@username 权限分配成功', array('@username' => $username)));
    return true;
  } else {
    dd_set_message(t('system', '@username 权限分配失败', array('@username' => $username)), 'error');
  }

}

/**
 * 取消用户所有权限，并删除其帐号 
 * @param string $username 
 *  用户名
 * @param string $target 
 *  连接名称
 * @access public
 * @return bool
 */
function db_drop_user($username, $target = 'default') {
  global $dbc;
  
  if (empty($dbc[$target])) db_connect($target);

  if (db_is_user($username, $target)) {
    if (call_user_func('db_' . $dbc[$target]['driver'].'_drop_user', $username, $target)) {
      dd_set_message(t('system', '@username 删除成功', array('@username' => $username)));
      return true;
    } else {
      dd_set_message(t('system', '@username 删除失败', array('@username' => $username)), 'error');
    }
  } else {
    dd_set_message(t('system', '@username 不存在', array('@username' => $username)), 'error');
  }

}

