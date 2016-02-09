<?php
// $Id$

/**
 * @Implement of hook_db_info()
 */
function mysql_db_info() {
	return array('mysql' => 'mysql');
}

/**
 * db_connect()
 * 创建数据库连接
 * @param (array) $dbc
 *  一组数据库连接信息，该信息由接口文件在 install.{type}.inc 中自行定义
 */
function db_mysql_connect($dbc, & $error) {
  $dsn = $dbc['driver'] .':host='. $dbc['host'].';port='.$dbc['port'];

  /**
   * 提供了数据库名，才选择数据库
   * @todo 在安装程序时，判断用户选择的数据库是否存在，不存在则创建。需要帐号有创建数据库权限，待完善
   */

  if (!empty($dbc['attributes'])) {
    $attributes = $dbc['attributes'];
  } else {
    $attributes = array(
      PDO::MYSQL_ATTR_USE_BUFFERED_QUERY => true,
      PDO::ATTR_PERSISTENT => true
    );
  }

  if (!empty($dbc['dbname'])) {
    $dsn .= ';dbname=' . $dbc['dbname'];
  }
  
  try {
    $db = new PDO($dsn, $dbc['dbuser'], $dbc['dbpass'], $attributes);
	  $db->exec('SET NAMES "utf8"');
	  $db->exec("SET sql_mode='ANSI,TRADITIONAL'");
  } catch (PDOException $e) {
  	$error = $e->getMessage();
  }

  return $db;
}

/**
 * db_query()
 * @example 基本查询：
 *  db_query("SELECT * FROM {test}");
 *
 * @example 带条件的查询：
 *  db_query("SELECT * FROM {test} WHERE id = :id", array(':id' => 10));
 *  db_query("SELECT * FROM {test} WHERE id = :id AND name = :name",array(':id' => 10, ':name' => $name));
 *  db_query("SELECT * FROM {test} WHERE id IN (:id, :id2)", array(':id' => 10, ':id2' => 12));
 *  db_query("SELECT * FROM {test} WHERE id IN (:id)", array(':id' => array(10, 20, 21));
 *
 * @example 带条件带属性的查询：
 *  db_query("SELECT * FROM {test} WHERE id = :id", array(':id' => 10), array('fetch' => 'array'));
 */
function db_mysql_query($sql, $args = array(), $opt = array()) {
  global $db;
  
  if ($opt['limit']) {
    if (is_array($opt['limit'])) {
      if (is_numeric($opt['limit'][0]) && is_numeric($opt['limit'][0])) {
        $sql .= ' LIMIT ' . $opt['limit'][0] . ' , ' . $opt['limit'][1];
      }
    } else if (is_numeric($opt['limit'])) {
      $sql .= ' LIMIT 0, ' . $opt['limit'];
    }
  }
  
  if (!empty($args)) {
    if ($smt = $db[$opt['target']]->prepare($sql)) {
      $smt->execute($args);
    }
  } else {
    $smt = $db[$opt['target']]->query($sql);
  }
  
  if (!is_object($smt)) {
    db_error(1111, array('db_mysql_query', 0, $sql));
    return false;
  }
  
  if (!db_error($smt->errorCode(), $smt->errorInfo())) {
    if (!$opt['fetch'] || $opt['fetch'] == 'object') {
      $opt['fetch'] = PDO::FETCH_OBJ;
    } else if ($opt['fetch'] == 'array') {
      $opt['fetch'] = PDO::FETCH_ASSOC;
    }
    
    $smt->setFetchMode($opt['fetch']);
    
    if (!$opt['return']) {
      return $smt->fetchAll();
    } else {
      switch ($opt['return']) {
        case 'one':
          return $smt->fetch();
        case 'column':
          return $smt->fetchColumn();
        case 'all':
          return $smt->fetchAll();
        case 'result':
          return $smt;
        case 'queryString':
        	return $smt->queryString;
        case 'return':
          return true;
      }
    }
  }
}

/**
 * db_replace()
 */
function db_mysql_replace($table, $args, array $opt = array()) { 
  $tables = module_get_schema($table);
  
  if (!is_array($tables['fields'])) {
    if (db_is_table($table)) {
      $tables = module_get_schema($table, 1);
    } else {
      dd_set_message(t('system', '无效的数据表 !string', array('!string' => $table)), 'error');
      return false;
    }
  }
  
  if (is_object($args)) {
    $args = (array) $args;
  }
  
  if (!is_array($args)) {
    dd_set_message(t('system', '待写入内容必须是对象或数组'), 'error');
    return false;
  }
  
  foreach ($args as $field_name => $value) {
    if (!isset($tables['fields'][$field_name])) {
      unset($args[$field_name]);
    } else {
      $placeholder = ':' . $field_name;
      $fields[] = $field_name;
      $placeholders[] = $placeholder;
      if (empty($tables['fields'][$field_name]['serialize'])) {
        $sql_args[$placeholder] = $value;
      } else {
        $sql_args[$placeholder] = serialize($value);
      }
    }
  }
  
  $sql = 'REPLACE INTO {' . $table . '} ';
  $sql .= '(' . implode(',', $fields) . ') VALUES ';
  $sql .= '(' . implode(',', $placeholders) . ')';
  
  return db_exec($sql, $sql_args, $opt);
}

/**
 * db_exec()
 * @example 写入一条
 *  db_exec("INSERT INTO {test} (name) VALUES (:value)", array(':value' => 'test'));
 *
 * @example 写入多条
 *  placeholder：占位符，若无此值，则视为默认，即 ?
 *  values：批量写入的值
 *   $args = array(
 *    'placeholder' => array(':value', ':name'),
 *    'values' => array(
 *      array('test1', 'test2'),
 *      array('test2', 'test2'),
 *      array('test3', 'test3'),
 *     )
 *  );
 *  db_exec("INSERT INTO {test} (value, name) VALUES (:value, :name)", $args);
 *
 * @example 删除
 *  db_exec('DELETE FROM {test} WHERE value = :value', array(':value' => 'test1'));
 *
 * @example 更新
 *  db_exec(
 *    'UPDATE {test} SET value = :value WHERE name = :name',
 *    array(':value' => 'test2', ':name' => 'test1')
 *  );
 */
function db_mysql_exec($sql, $args = array(), $opt = array()) {
  global $db;
  static $transaction;
  
  if (!empty($opt['transaction'])) {
    switch ($opt['transaction']) {
      case 'start':
        $db[$opt['target']]->beginTransaction();
        $transaction = true;
        
        if (empty($sql)) return true;
      break;
      case 'end':
        $db[$opt['target']]->commit();
        $transaction = false;
        
        if (empty($sql)) return true;
      break;
      case 'back':
        $db[$opt['target']]->rollBack();
        $transaction = false;
        
        return true;
    }
  }
  
  $db[$opt['target']]->setAttribute(PDO::MYSQL_ATTR_USE_BUFFERED_QUERY, false);
  
  $rowCount = 0;
  
  if ($args) {
    $smt = $db[$opt['target']]->prepare($sql);
    if (!$args['values']) {
      if ($smt->execute($args)) {
        $rowCount = $smt->rowCount();
      } else {
      	db_error($smt->errorCode(), $smt->errorInfo());
        if ($opt['transaction']) {
          $db[$opt['target']]->rollBack();
        }
      }
    } else {
      foreach ($args['values'] as $values) {
        if (!$args['placeholder']) {
          if (!is_array($values)) {
            $values = array($values);
          }
          $smt->execute($values);
        } else {
          $smt->execute(array_combine($args['placeholder'], $values));
        }
        
        if (!db_error($smt->errorCode(), $smt->errorInfo())) {
          $rowCount += $smt->rowCount();
        } else if ($opt['transaction']) {
          $db[$opt['target']]->rollBack();
        }
      }
    }
  } else {
    try{
      $rowCount = $db[$opt['target']]->exec($sql);
    }catch (Exception $e) {
      if ($opt['transaction']) {
        $db[$opt['target']]->rollBack();
      }
    }
  }
  
  if (!$opt['return']) {
    return $rowCount;
  } else if (isset($rowCount)) {
    /**
     * 有时候 update 没有受影响的列数，但请求者想要知道语句是否成功执行。
     * 若没有受影响的列数，返回 true，只表示语句已成功执行
     */
    return $rowCount ? $rowCount : true;
  }
}

/**
 * @Implement of db_show_tables()
 */
function db_mysql_show_tables($target) {
  global $db, $dbc;
  
  $prefix = $dbc[$target]['prefix'];
  
  if ($smt = $db[$target]->query('SHOW TABLES')) {
    foreach ($smt->fetchAll(PDO::FETCH_ASSOC) as $table) {
      $name = current($table);
      
      if ($prefix && substr($name, 0, strlen($prefix)) == $prefix) {
        $name = substr($name, strlen($prefix));
      }
      
      $tables[] = $name;
    }
    return $tables;
  }
}

/**
 * @Implement of db_get_version()
 */
function db_mysql_get_version($target) {
  global $db;
  if ($smt = $db[$target]->query('SELECT version()')) {
    return $smt->fetchColumn();
  }
}

/**
 * @Implement of db_is_table
 */
function db_mysql_is_table($table, $target) {
  return (bool) db_query("SHOW TABLES LIKE '{".$table."}'", NULL, array('target' => $target, 'return' => 'one'));
}


/**
 * @Implement of db_is_field
 */
function db_mysql_is_field($table, $field, $target) {
  return (bool) db_query('DESCRIBE {'.$table.'} ?', array($field), array('target' => $target));
}

/**
 * @Implement of db_drop_table()
 */
function db_mysql_drop_table($table, $target) {
  
  return db_query('DROP TABLE {'.$table.'}', NULL, array('target' => $target, 'return' => 'return'));
}

/**
 * @Implement of db_rename_table()
 */
function db_mysql_rename_table($name, $new_name, $target) {
	return db_query('ALTER TABLE {'. $name .'} RENAME TO {'. $new_name .'}', NULL, array('target' => $target, 'return' => 'return'));
}

/**
 * @Implement of db_field_set_default()
 */
function db_mysql_field_set_default($table, $field, $value, $target) {
  $return = db_query('ALTER TABLE {'. $table .'} ALTER COLUMN '. $field .' SET DEFAULT ?', array($value), array('target' => $target));
  return isset($return);
}

/**
 * @Implement of db_field_drop_default()
 */
function db_mysql_field_drop_default($table, $field, $target) {
  return db_query('ALTER TABLE {'. $table .'} ALTER COLUMN '. $field .' DROP DEFAULT', NULL, array('target' => $target, 'return' => 'return'));
}

/**
 * @Implement of db_add_primary()
 */
function db_mysql_add_primary($table, $fields, $target) {
  return db_query('ALTER TABLE {'. $table .'} ADD PRIMARY KEY ('.
    _db_mysql_create_key_sql($fields) .')', NULL, array('target' => $target, 'return' => 'return'));
}

/**
 * @Implement of db_drop_primary()
 */
function db_mysql_drop_primary($table, $target) {
  return db_query('ALTER TABLE {'. $table .'} DROP PRIMARY KEY', NULL, array('target' => $target, 'return' => 'return'));
}

/**
 * @Implement of db_add_unique_key()
 */
function db_mysql_add_unique($table, $name, $fields, $target) {
  if ($queryString = db_query('ALTER TABLE {'. $table .'} ADD UNIQUE KEY '.
    $name .' ('. _db_mysql_create_key_sql($fields) .')', NULL, array('target' => $target, 'return' => 'queryString'))) {
    return array($queryString);
  }
}

/**
 * @Implement of db_drop_unique()
 */
function db_mysql_drop_unique($table, $name, $target) {
  if ($queryString = db_query('ALTER TABLE {'. $table .'} DROP KEY '.
    $name, NULL, array('target' => $target, 'return' => 'queryString'))) {
    return array($queryString);
  }
}

/**
 * @Implement of db_add_index()
 */
function db_mysql_add_index($table, $name, $fields, $target) {
  if ($queryString = db_query('ALTER TABLE {'. $table .'} ADD INDEX '.
    $name .' ('. _db_mysql_create_key_sql($fields) .')', NULL, array('target' => $target, 'return' => 'queryString'))) {
    return array($queryString);
  }
}

/**
 * @Implement of db_drop_index()
 */
function db_mysql_drop_index($table, $name, $target) {
  if ($queryString = db_query('ALTER TABLE {'. $table .'} DROP INDEX '.
    $name, NULL, array('target' => $target, 'return' => 'queryString'))) {
    return array($queryString);
  }
}

/**
 * @Implement of db_add_field()
 */
function db_mysql_add_field($table, $field, $spec, $keys_new = array(), $target = 'default') {
  $fixnull = false;
  if (!empty($spec['not null']) && !isset($spec['default'])) {
    $fixnull = true;
    $spec['not null'] = false;
  }
  
  $query = 'ALTER TABLE {'. $table .'} ADD ';
  $query .= _db_mysql_create_field_sql($field, _db_mysql_process_field($spec), $target);
  if (count($keys_new)) {
    $query .= ', ADD '. implode(', ADD ', _db_mysql_create_field_sql($keys_new));
  }
  
  if ($queryString = db_query($query, NULL, array('target' => $target, 'return' => 'queryString'))) {
    $sql[] = $queryString;
    if (isset($spec['initial'])) {
      if ($queryString = db_query('UPDATE {'. $table .'} SET '. $field .' = ?', array($spec['initial']), array('target' => $target, 'return' => 'queryString'))) {
        $sql[] = $queryString;
      }
    }
    if ($fixnull) {
      $spec['not null'] = true;
      $sql[] = db_mysql_change_field($table, $field, $field, $spec, $target);
    }
    return $sql;
  }
}

/**
 * @Implement of db_drop_field()
 */
function db_mysql_drop_field($table, $field, $target) {
  if ($queryString = db_query('ALTER TABLE {'. $table .'} DROP '. $field, NULL, array('target' => $target, 'return' => 'queryString'))) {
    return array($queryString);
  }
}

/**
 * @Implement of db_add_foreign()
 */
function db_mysql_add_foreign($table, $field, $foreign, $target) {
	if (!db_is_table($table, $target)) {
		dd_set_message(t('system', '表 @table 不存在', array('@table' => $table)), 'error');
	} else if (db_is_table($foreign['table'], $target)) {
		if ($foreign['field']) {
			$sql = 'ALTER TABLE {'.$table."} ADD CONSTRAINT \n";
			$sql .= 'f_'.$table.'_'.$foreign['table'].'_'.$field.'_'.$foreign['field'] . "\n";
			$sql .= 'FOREIGN KEY ('.$field.")\n";
			$sql .= 'REFERENCES {'.$foreign['table'].'}('.$foreign['field'].")\n";
			$sql .= 'ON UPDATE ' . _db_mysql_add_foreign_bind($foreign['update']). "\n";
			$sql .= 'ON DELETE '._db_mysql_add_foreign_bind($foreign['delete']);
			
			if ($queryString = db_query($sql, NULL, array('target' => $target, 'return' => 'queryString'))) {
				return $queryString;
			}
			
		} else {
			dd_set_message(t('system', '必须提供主表字段值'), 'error');
		}
		
	} else {
		dd_set_message(t('system', '表 @table 不存在', array('@table' => $foreign['table'])), 'error');
  }
}

/**
 * @Implement of db_drop_foreign()
 */
function db_mysql_drop_foreign($table, $field, $parent_table, $parent_field, $target = 'default') {
  $key = 'f_'.$table.'_'.$parent_table.'_'.$field.'_'.$parent_field;
  
  if ($queryString = db_query('ALTER TABLE {'. $table .'} DROP FOREIGN KEY '.
    $key, NULL, array('target' => $target, 'return' => 'queryString'))) {
    
    @db_query('ALTER TABLE {'. $table .'} DROP INDEX '.
    $key, NULL, array('target' => $target, 'return' => 'queryString'));
    
    return array($queryString);
  }
}

function db_mysql_change_field($table, $field, $field_new, $spec, $keys_new, $target) {
  $sql = 'ALTER TABLE {'. $table .'} CHANGE '. $field .' '.
    _db_mysql_create_field_sql($field_new, _db_mysql_process_field($spec), $target);
  if (count($keys_new)) {
    $sql .= ', ADD '. implode(', ADD ', _db_mysql_create_keys_sql($keys_new));
  }
  if ($queryString = db_query($sql, NULL, array('target' => $target, 'return' => 'queryString'))) {
    return array($queryString);
  }
}

/**
 * @Implement of db_create_table()
 */
function db_mysql_create_table($name, $table, $target) {
  $table += array(
    'mysql_engine' => 'InnoDB',
    'mysql_character_set' => 'UTF8',
  );
  
  $sql = 'CREATE TABLE {' . $name . "} (\n";
  
  foreach ($table['fields'] as $field_name => $field) {
    $sql .= _db_mysql_create_field_sql($field_name, _db_mysql_process_field($field), $target) . ", \n";
  }
  
  $keys = _db_mysql_create_keys_sql($table);
  if (count($keys)) {
    $sql .= implode(", \n", $keys) . ", \n";
  }
  
  $sql = substr($sql, 0, -3) . "\n) ";
  
  $sql .= 'ENGINE = ' . $table['mysql_engine'] . ' DEFAULT CHARACTER SET ' . $table['mysql_character_set'];
  
  if (!empty($table['description'])) {
    $sql .= ' COMMENT ' . db_mysql_get_comment($table['description'], $target);
  }
  
	if ($queryString = db_query($sql, NULL, array('target' => $target, 'return' => 'queryString'))) {
		
		$return[] = $queryString;
		
		if ($table['foreign keys'] && is_array($table['foreign keys'])) {
			foreach ($table['foreign keys'] as $key => $foreign) {
				$return[] = db_mysql_add_foreign($name, $key, $foreign, $target);
			}
		}
   return $return;
 }
}

/**
 * 解析创建表语句
 */
function _db_mysql_create_field_sql($name, $spec, $target) {
  $sql = "`". $name ."` ". $spec['mysql_type'];

  if (isset($spec['length'])) {
    $sql .= '('. $spec['length'] .')';
  } elseif (isset($spec['precision']) && isset($spec['scale'])) {
    $sql .= '('. $spec['precision'] .', '. $spec['scale'] .')';
  }

  if (!empty($spec['unsigned'])) {
    $sql .= ' unsigned';
  }

  if (!empty($spec['not null'])) {
    $sql .= ' NOT NULL';
  }

  if (!empty($spec['auto_increment'])) {
    $sql .= ' auto_increment';
  }

  if (isset($spec['default'])) {
    if (is_string($spec['default'])) {
      $spec['default'] = "'". $spec['default'] ."'";
    }
    $sql .= ' DEFAULT '. $spec['default'];
  }

  if (empty($spec['not null']) && !isset($spec['default'])) {
    $sql .= ' DEFAULT NULL';
  }
  
  if ($spec['description']) {
    $sql .= ' COMMENT ' . db_mysql_get_comment($spec['description'], $target);
  }
  
  return $sql;
}

/**
 * 获取字段类型
 */
function _db_mysql_process_field($field) {
  if (!isset($field['size'])) {
    $field['size'] = 'normal';
  }

  if (!isset($field['mysql_type'])) {
    $map = db_mysql_type_map();
    $field['mysql_type'] = $map[$field['type'] . ':' . $field['size']];
  }

  if ($field['type'] == 'serial') {
    $field['auto_increment'] = true;
  }
  
  return $field;
}

/**
 * 解析表主键、索引、唯一值
 */
function _db_mysql_create_keys_sql($spec) {
  $keys = array();

  if (!empty($spec['primary key'])) {
    $keys[] = 'PRIMARY KEY (' . _db_mysql_create_key_sql($spec['primary key']) . ')';
  }
  if (!empty($spec['unique keys'])) {
    foreach ($spec['unique keys'] as $key => $fields) {
      $keys[] = 'UNIQUE KEY `' . $key . '` (' . _db_mysql_create_key_sql($fields) . ')';
    }
  }
  if (!empty($spec['indexes'])) {
    foreach ($spec['indexes'] as $index => $fields) {
      $keys[] = 'INDEX `' . $index . '` (' . _db_mysql_create_key_sql($fields) . ')';
    }
  }

  return $keys;
}

function _db_mysql_create_key_sql($fields) {
  $return = array();
  foreach ($fields as $field) {
    if (is_array($field)) {
      $return[] = '`' . $field[0] . '`(' . $field[1] . ')';
    } else {
      $return[] = '`' . $field . '`';
    }
  }
  return implode(', ', $return);
}

/**
 * 外键约束行为
 */
function _db_mysql_add_foreign_bind($type) {
	switch ($type) {
		case 'null': // 子表设置为空
			return 'SET NULL';
		case 'default': // 子表设置默认值
			return 'SET DEFAULT';
		case 'action': // 若子表存在关联值，主表禁止删除
			return 'NO ACTION';
		default: case 'cascade': // 子表同步
			return 'CASCADE';
	}
}

function db_mysql_type_map() {
  static $map = array(
    'varchar:normal'  => 'VARCHAR',
    'char:normal'     => 'CHAR',

    'text:tiny'       => 'TINYTEXT',
    'text:small'      => 'TINYTEXT',
    'text:medium'     => 'MEDIUMTEXT',
    'text:big'        => 'LONGTEXT',
    'text:normal'     => 'TEXT',

    'serial:tiny'     => 'TINYINT',
    'serial:small'    => 'SMALLINT',
    'serial:medium'   => 'MEDIUMINT',
    'serial:big'      => 'BIGINT',
    'serial:normal'   => 'INT',

    'int:tiny'        => 'TINYINT',
    'int:small'       => 'SMALLINT',
    'int:medium'      => 'MEDIUMINT',
    'int:big'         => 'BIGINT',
    'int:normal'      => 'INT',

    'float:tiny'      => 'FLOAT',
    'float:small'     => 'FLOAT',
    'float:medium'    => 'FLOAT',
    'float:big'       => 'DOUBLE',
    'float:normal'    => 'FLOAT',

    'numeric:normal'  => 'DECIMAL',

    'blob:big'        => 'LONGBLOB',
    'blob:normal'     => 'BLOB',

    'date:normal'     => 'DATE',

    'datetime:normal' => 'DATETIME',

    'time:normal'     => 'TIME',
  );
  return $map;
}

/**
 * 数据库注释文字解析 
 * @param string $comment 
 *  注释文字
 * @param string $target 
 *  连接名称
 * @access public
 * @return string
 */
function db_mysql_get_comment($comment, $target) {
  $comment = str_replace("'", '’', $comment);
  return $GLOBALS['db'][$target]->quote($comment);
}

/**
 * @Implement of db_is_database
 * 连接数据库帐号须拥有全局 show databases 权限，才能正确判断是否存在
 */
function db_mysql_is_database($database, $target) {
  return (bool) db_query('SHOW DATABASES LIKE ?', array($database), array('target' => $target, 'return' => 'column'));
}

/**
 * @Implement of db_create_database()
 * 连接帐号必须拥有创建数据库权限  
 */
function db_mysql_create_database($database, $target) {
  return (bool) db_exec('CREATE DATABASE IF NOT EXISTS ' . $database, NULL, array('target' => $target));
}

/**
 * @Implement of db_drop_database
 * 连接帐号必须拥有删除数据库的权限
 */
function db_mysql_drop_database($database, $target) {
  return (bool) db_exec('DROP DATABASE IF EXISTS ' . $database, NULL, array('target' => $target));
}

/**
 * @Implement of db_is_user()
 * 连接帐号必须拥有 mysql 表查询权限
 */
function db_mysql_is_user($username, $target) {
  return (bool) db_query('SELECT COUNT(*) FROM `mysql`.`user` WHERE User = ?',
  array($username), array('target' => $target, 'return' => 'column'));
}

/**
 * @Implement of db_create_user()
 * 连接帐号必须要全局插入权限
 */
function db_mysql_create_user($username, $password, $database, $host, $target) {
  db_query('GRANT ALL PRIVILEGES ON ' . $database . '.* TO ' . $username . "@'" . $host 
  . "' IDENTIFIED BY '" . $password . "'", NULL, array('target' => $target));
  return db_is_user($username, $target);
}

/**
 * @Implement of db_drop_user
 * 连接帐号必须拥有全局删除用户的权限
 */
function db_mysql_drop_user($username, $target) {
  db_exec('DROP USER ' . $username, NULL, array('target' => $target));
  if (!db_is_user($username, $target)) {
    // 刷新权限
    db_query('FLUSH PRIVILEGES', NULL, array('target' => $target));
    return true;
  }
}
