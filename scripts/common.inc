<?php
// $Id$

/**
 * 脚本开始运行时间
 */
define('DIDA_START_TIME', microtime(true));

/**
 * @file 从命令行执行脚本
 *
 * 加载 bootstrap.inc
 * 设置一个全局变量 $_args，即解析为数据的命令行参数
 *
 * 调用本文件，必须在命令行下指定一个站点参数，即 --host www.test.com
 *
 * 示例 1，加载 user 模块下的 script.inc 脚本：
 *  php common.inc --host www.test.com --module_scripts user --script_name script.inc
 */

/**
 * 
 * 将命令行参数解析为数组，约定使用 --
 * 如：php test.php --m test --k --p a -p b
 * 解析为 array('m' => 'test', 'k' => true, 'p' => array('a', 'b'));
 * 如：php test.php --m test --test[a] 1 --test[b] 2
 * 解析为 array('m' => 'test', 'test' => array('a' => 1, 'b' => 2));
 * 
 * @param string $key
 * 	获取指定参数
 * @param string $argc
 *  指定参数长度，通常用于代码中调用本函数
 * @param array $argv
 *  指定待解析的数组，通常用于代码中调用本函数
 * @return array|*
 */
function args($key = NULL, $argc = NULL, $argv = NULL) {

  if (empty($argc)) $argc = $_SERVER['argc'];
  if (empty($argv)) $argv = $_SERVER['argv'];

  $_args = array();

  for ($i = 1; $i < $argc; ++$i) {
    if (preg_match('/--(\S+)/', $argv[$i], $mat)) {
      $i++;
      if (substr($argv[$i], 0, 2) != '--') {
        $_args[$mat[1]][] = $argv[$i];
      } else {
        $i--;
        $_args[$mat[1]][] = true;
      }
    } else {
      $_args[$i] = $argv[$i];
    }
  }
  
  $args = array();
  foreach ($_args as $_key => $v) {
    if (preg_match('/\[(.*?)\]/', $_key, $mat)) {
      $k = str_replace('[' . $mat[1] . ']', '', $_key);
      if (empty($args[$k]) || !is_array($args[$k])) {
        $args[$k] = array();
      }
      $args[$k][$mat[1]] = is_array($v) && count($v) == 1 ? $v[0] : $v;
    } else if (is_array($v) && count($v) == 1) {;
      $args[$_key] = $v[0];
    } else {
      $args[$_key] = $v;
    }
  }
  
  unset($_args);

  return isset($key) ? $args[$key] : $args;
}

/**
 * 输出到终端的提示信息 
 * @access public
 * @return string
 */
function message() {
  $args = func_get_args();

  ob_start();
  print_r($args);
  $text = ob_get_contents();
  ob_end_clean();

  if (empty($_SERVER['LANG']) || stripos($_SERVER['LANG'], 'utf') === false) {
    $text = iconv('UTF-8', 'GBK', $text);
  }

  $text .= "\n";

  echo $text;
}

// 解析命令行参数
$_args = args();

if (empty($_args['host'])) {
  print("必须指定站点，如：www.cmsns.com，才能加载配置文件\n");
  exit();
}

// 设置工作目录
define('DIDA_ROOT', dirname(dirname(__FILE__)));
chdir(DIDA_ROOT);

set_time_limit(0);

// 使用指定站点的配置文件，必须有这个参数
$_SERVER['HTTP_HOST'] = $_args['host'];
$_SERVER['SCRIPT_NAME'] = '/index.php';

// url 参数，模拟为首页
$_SERVER['REQUEST_URI'] = '/';

// 指定 ip
$_SERVER['REMOTE_ADDR'] = '127.0.0.1';

require_once DIDA_ROOT . '/includes/bootstrap.inc';

// 初始化程序
if (!empty($_args['bootstrap']) || !empty($is_cron)) {
  bootstrap((!empty($_args['bootstrap']) && $_args['bootstrap'] == 'data' ? 'data' : 'full'));
}

/**
 * 终端下执行，解析得到的 $base_path 会加上 scripts 目录，重写
 */
$base_url = str_replace('/scripts', '', $base_url);
$base_path = str_replace('/scripts/', '', $base_path);

/**
 * 某些时候，涉及到大量数据操作的功能，耗时较长，可能需要在命令行下执行。
 * 模块可自行添加脚本，通过统一格式进行调用执行，如：
 *  sites/modules/mymodule/scripts/myscript.inc
 * 以如下方式调用执行：
 *  php -f common.inc --host www.cmsns.com --module_scripts mymodule --script_name myscript.inc
 */

// 需要加载的脚本文件名称
if (!empty($_args['script_name'])) {

  // 默认脚本文件目录为 scripts，可以指定一个模块名称
  if (!empty($_args['module_scripts'])) {
    $script_path = DIDA_ROOT . '/' . dd_get_path('module', $_args['module_scripts']) . '/scripts/' . $_args['script_name'];
  } else {
    $script_path = DIDA_ROOT . '/scripts/' . $_args['script_name'];
  }
  
  if (is_file($script_path)) {
    require_once $script_path;
    // 输出 session 中的提示信息
    echo strip_tags(dd_get_message());
  }
}

