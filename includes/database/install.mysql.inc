<?php
// $Id$

/**
 * 数据库连接信息，由各数据库接口文件自定义
 */
function db_install_form($database) {
  $field = array();

  $field[] = array(
    '#type' => 'textfield',
    '#name' => 'dbname',
    '#title' => '数据库名称',
    '#required' => 1,
    '#value' => $database['dbname']
  );

  $field[] = array(
    '#type' => 'textfield',
    '#name' => 'dbuser',
    '#title' => '数据库用户名',
    '#required' => 1,
    '#value' => $database['dbuser']
  );

  $field[] = array(
    '#type' => 'password',
    '#name' => 'dbpass',
    '#title' => '数据库密码',
    '#value' => $database['dbpass']
  );

  $field[] = array(
    '#type' => 'textfield',
    '#name' => 'host',
    '#title' => '数据库主机',
    '#required' => 1,
    '#value' => $database['host'] ? $database['host'] : 'localhost'
  );

  $field[] = array(
    '#type' => 'textfield',
    '#name' => 'port',
    '#title' => '数据库端口',
    '#required' => 1,
    '#value' => $database['port'] ? $database['port'] : 3306
  );

  $field[] = array(
    '#type' => 'textfield',
    '#name' => 'prefix',
    '#title' => '数据库前缀',
    '#value' => $database['prefix']
  );

  return $field;
}

/**
 * 测试数据库权限，若通过，则保存配置到 setting.php
 * 测试与保存由各数据库接口文件自定义
 */
function db_install_test($database) {
  if ($database['dbuser'] && $database['dbname'] && $database['host']) {
    $dsn = $database['driver'] .':host='. $database['host'].';port='.$database['port'].';dbname='.$database['dbname'];
    try{
      $db = new PDO($dsn, $database['dbuser'], $database['dbpass']);
  	  $db->exec('SET NAMES "utf8"');
  	  $db->exec("SET sql_mode='ANSI,TRADITIONAL'");
  	  /**
  	   * 安装文件仅测试： 增删查，创建、删除、修改等权限。
  	   * 根据不同数据库特性，更多权限在此自定义测试，并返回错误信息以阻止安装
  	   */
    }catch(PDOException $e) {
      return array($e->getMessage());
    }
  } else {
    $error[] = '数据库信息不完整';
  }
  return $error;
}
