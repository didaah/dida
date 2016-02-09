<?php
// $Id$

define('DIDA_VERSION', '1.0 alpha 3');

/**
 * 系统初始化
 * @param (string) $op
 *  加载类型
 * @param (array) $inc
 *  加载文件
 */
function bootstrap($op, $inc = NULL) {
  static $called;

  if ($called) return;

  date_default_timezone_set('Asia/Chongqing');

  global $cookie_domain, $user, $database, $dbc, $db, $conf, $conf_dir, $log_dir, $installed, $free_update, $_DGET;

  $log_dir = DIDA_ROOT . '/sites/logs';

  if (is_file(DIDA_ROOT . '/sites/config.php')) {
    require_once DIDA_ROOT . '/sites/config.php';
    if (isset($config) && !empty($config[$_SERVER['HTTP_HOST']])) {
      $conf_dir = $config[$_SERVER['HTTP_HOST']];
    } else if (!empty($config['default'])) {
      $conf_dir = $config['default'];
    }
  }

  if (!$conf_dir) {
    if (is_dir(DIDA_ROOT . '/sites/' . $_SERVER['HTTP_HOST'])) {
      $conf_dir = 'sites/' . $_SERVER['HTTP_HOST'];
    } else {
      $conf_dir = 'sites';
    }
  }

  if (!is_file(DIDA_ROOT . '/' . $conf_dir . '/setting.php')) {
    dd_to_install();
  }

  require_once DIDA_ROOT . '/' . $conf_dir . '/setting.php';

  if (!$installed) dd_to_install();

  if (is_file(DIDA_ROOT . '/' . $conf_dir . '/cache/conf.php')) {
    require_once DIDA_ROOT . '/' . $conf_dir . '/cache/conf.php';
  }

  // 没有默认主题，通常是配置文件损坏，重新生成
  if (empty($conf) || empty($conf['default_theme'])) {
    require_once DIDA_ROOT . '/includes/database.inc';
    //初始化默认数据库连接
    db_connect('default');
    var_init();
    require_once DIDA_ROOT . '/' . $conf_dir . '/cache/conf.php';
  }

  // 若限制了域名，则验证 HTTP_HOST
  if (!empty($conf['default_host'])) {
    if ($_hosts = explode(',', trim($conf['default_host']))) {
      if (!in_array($_SERVER['HTTP_HOST'], $_hosts)) {
        dd_goto('http://' . $_hosts[0] . $_SERVER['REQUEST_URI']);
      }
    }
  }

  set_error_handler('dd_php_error');

  switch ($op) {
    case 'full':
      require_once DIDA_ROOT . '/includes/database.inc';

      //初始化默认数据库连接
      db_connect('default');

      // session 及配置
      conf_init();

      require_once DIDA_ROOT . '/includes/filter.inc';
      require_once DIDA_ROOT . '/includes/theme.inc';
      require_once DIDA_ROOT . '/includes/image.inc';
      require_once DIDA_ROOT . '/includes/form.inc';
      require_once DIDA_ROOT . '/includes/file.inc';
      if (is_file(DIDA_ROOT . var_get('dd_cache_file'))) {
        require_once DIDA_ROOT . var_get('dd_cache_file');
      } else {
        require_once DIDA_ROOT . '/includes/cache.inc';
      }
      require_once DIDA_ROOT . '/includes/module.inc';
      require_once DIDA_ROOT . '/includes/menu.inc';
      require_once DIDA_ROOT . '/includes/pager.inc';

      if (!empty($_GET)) {
        foreach ($_GET as $key => $value) {
          $_DGET[$key] = $value;
          $_GET[$key] = menu_xss_filter($value);
        }
        $_SERVER['_REQUEST_URI'] = $_SERVER['REQUEST_URI'];
        $_SERVER['REQUEST_URI'] = menu_xss_filter($_SERVER['REQUEST_URI']);
      }

      // 载入模块主文件，将执行 hook_bootstrap()
      module_bootstrap();

      //用户验证
      dd_user_validate();

      dd_page_header();

      //载入主题
      theme_init();

      // 模块初始化
      module_init();

      /**
       * 非其它渠道验证请求
       * 比如通过 flash 上传，浏览器和 flash cookie 不一致，可能导致 session 错误
       * 当请求中包含特定 POST 参数，视作验证请求，通过验证后，将根据参数加载相应用户记录
       * 该请求视作非正常请求，不记录活动状态
       */
      $user->__SETVALIDATE = NULL;

      if (!empty($_POST['__SETPHPSESSID']) && !empty($_POST['__SETUID'])) {
        if (!empty($user->uid)) {
          if ($_POST['__SETUID'] != $user->uid) {
            dd_get_access();
            exit;
          } else {
            $user->__SETVALIDATE = time();
          }
        } else if (db_query('SELECT name FROM {users} WHERE session = ? AND uid = ? AND host = ?',
        array($_POST['__SETPHPSESSID'], $_POST['__SETUID'], ip_address()))) {
          if ($user = user_login(user_load($_POST['__SETUID']))) {
            $user->__SETVALIDATE = time();
          }
        }
      }

      switch ($conf['status']) {
        case 2: //登录后可浏览
          if (empty($user->uid) && $_GET['q'] != 'user/login') dd_get_online();
          break;
        case 0:
        case 3: //超级管理员可浏览
          if ($user->uid != 1 && $_GET['q'] != 'user/login') dd_get_online();
      }

      // 浏览记录，记录每个用户的每次访问
      if (var_get('history_enabled', 0)) {
        db_exec('INSERT INTO {history} (uid, session, host, timestamp, referer, path) VALUES
          (?, ?, ?, ?, ?, ?)', array($user->uid, session_id(), ip_address(), $_SERVER['REQUEST_TIME'],
        dd_get_referer($_SERVER['HTTP_REFERER']), dd_get_referer($_SERVER['REQUEST_URI'])));
      }

    break;
    case 'data':
      /**
       * 仅载入数据库连接
       */
      require_once DIDA_ROOT . '/includes/database.inc';
      require_once DIDA_ROOT . '/includes/pager.inc';

      //初始化默认数据库连接
      db_connect('default');

    break;
    case 'custom':
      /**
       * 跳过所有模块，默认仅加载数据库及缓存接口文件
       * 提供数据库连接，用户验证，全局配置
       * 可自定义载入其它核心文件
       */
      require_once DIDA_ROOT . '/includes/database.inc';
      //初始化默认数据库连接
      db_connect('default');

      // session 及配置
      conf_init();

      if (is_file(DIDA_ROOT . var_get('dd_cache_file'))) {
        require_once DIDA_ROOT . var_get('dd_cache_file');
      } else {
        require_once DIDA_ROOT . '/includes/cache.inc';
      }

      if (is_array($inc)) {
        foreach ($inc as $filename) {
          $filepath = DIDA_ROOT . '/includes/'.$filename.'.inc';
          if (is_file($filepath)) require_once $filepath;
        }
      }

      // 载入模块主文件，将执行 hook_bootstrap()
      module_bootstrap();

      //用户验证
      dd_user_validate();

      switch ($conf['status']) {
        case 2: //登录后可浏览
          if (!$user->uid && $_GET['q'] != 'user/login') dd_get_online();
          break;
        case 3: //超级管理员可浏览
          if ($user->uid != 1 && $_GET['q'] != 'user/login') dd_get_online();
      }
  }
  $called = 1;
}

/**
 * 获取模块、主题文件物理路径
 * @param string $type
 *  类型，分为：theme、module、all
 * @param (string) $name
 *  模块或主题的系统名称
 */
function dd_get_path($type, $name) {
  global $conf;

  switch ($type) {
    case 'theme':
      $lists = $conf['themes'];
    break;
    case 'all': 
      $lists = $conf['module_theme']; // 模块或主题
    break;
    case 'module': default:
      $lists = $conf['modules'];
    break;
  }

  return !empty($lists[$name]['path']) ? $lists[$name]['path'] : false;
}

/**
 * 写入一个任务到cron表，在调度 cron 时，优先执行
 * @param (string) $module
 *  模块系统名称，错误的名称将无法写入
 * @param (string) $type
 *  任务类型，由模块自定义，便于自行查找
 * @param (array) $data
 *  任务数据。必须包含回调函数。参数(*表示必须，以下同)：
 *    *(string) func：执行该条任务时，将回调此函数。返回 true 为成功
 *    (array) args：传递给回调函数的参数
 *    (array) includes：需要加载的文件，完整的系统路径
 *    (string) success：成功执行回调的函数，并以 args 作为参数传递。
 * @param (int) $weight
 *  任务权重，可提高任务优先级。排序方式：weight ASC, cid ASC
 * @return
 *  写入成功则返回 cid
 */
function dd_save_cron($module, $type, $data = array(), $weight = 0) {
  if ($GLOBALS['conf']['modules'][$module]) {
    if (db_exec('INSERT INTO {cron} (module, type, data, weight) VALUES (?, ?, ?, ?)',array($module, $type, serialize($data), $weight))) {
      return db_last_insert_id();
    }
  }
}

/**
 * 站点关闭
 */
function dd_get_online() {
  header('Content-Type: text/html; charset=utf-8');
  if ($output = @file_get_contents($GLOBALS['conf_dir'] . '/cache/site_offline_html.lang')) {
    echo $output;
  } else {
    echo '站点维护中…';
  }
  exit;
}

/**
 * 获取完整 URL
 */
function dd_get_absolute_url() {
  return $GLOBALS['base_url'] . $_SERVER['REQUEST_URI'];
}

/**
 * 页面不存在
 */
function dd_get_not() {
  header('HTTP/1.1 404 Not Found');
  if ($GLOBALS['conf']['page_not_history']) {
    dd_set_message(t('system', '抱歉，%string 不存在', array('%string' => dd_get_absolute_url())), 'error');
    dd_goto(dd_get_history());
  } else {
    $text = t('system', '访问的页面不存在');
    dd_set_title(array($text));
    $default = '<h2 align="center">' . l($text, NULL) . '</h2>';
    echo theme('error', custom_get('page_not_html', $default), 404);
  }
  exit;
}

/**
 * 没有访问权限，如未登录，则显示登录表单
 */
function dd_get_access() {
  global $user;
  if ($user->uid > 0) {
    header('HTTP/1.1 403 Forbidden');
    if ($GLOBALS['conf']['page_access_history']) {
      dd_set_message(t('system', '抱歉，没有权限访问 %string', array('%string' => dd_get_absolute_url())), 'error');
      dd_goto(dd_get_history());
    } else {
      $text = t('system', '没有权限访问该页');
      dd_set_title(array($text));
      $default = '<h2 align="center">' . l($text, NULL) . '</h2>';
      echo theme('print', custom_get('page_access_html', $default));
    }
  } else {
    $title = t('system', '登录以确认访问权限');
    dd_set_title(array($title));
    echo theme('error', dd_get_form('user_login_form'), 403);
  }
  exit;
}

/**
 * 上一页地址，若上一页非当前网站，则返回首页
 */
function dd_get_history() {
  if ($_SERVER['HTTP_REFERER'] && strpos($_SERVER['HTTP_REFERER'], $GLOBALS['base_url']) !== false) {
    return $_SERVER['HTTP_REFERER'];
  } else {
    return $GLOBALS['base_url'];
  }
}

/**
 * 注销所有全局变量
 */
function dd_unset_globals() {
  if (ini_get('register_globals')) {
    $allowed = array('_ENV' => 1, '_GET' => 1, '_POST' => 1, '_COOKIE' => 1, '_FILES' => 1, '_SERVER' => 1, '_REQUEST' => 1, 'GLOBALS' => 1);
    foreach ($GLOBALS as $key => $value) {
      if (!isset($allowed[$key])) {
        unset($GLOBALS[$key]);
      }
    }
  }
}

/**
 * 重定向
 * @param (string) $path
 *  系统路径或完整 url
 * @param (array or string) $query
 *  query 参数，字符串或数组形式
 * @param (bool) $file
 *  是否物理文件
 * @param (string) $fragment
 *  锚点
 * @param (int) $http_response_code
 *  重定向模式，301 或 302
 */
function dd_goto($path = '', $query = NULL, $file = NULL, $fragment = NULL, $http_response_code = 301) {
  $url = url($path, array('query' => $query, 'fragment' => $fragment, 'absolute' => true, 'file' => $file));
  session_write_close();
  dd_page_header();
  header('Location: '. $url, true, $http_response_code);
  exit();
}

/**
 * 超链接格式化
 * @param (string) $text
 *  链接名称
 * @param (string) $path
 *  系统路径或完整 url
 * @param $options
 *  详细参数：
 *  array(
 *    'attributes' => (array), // a 链接属性，如 'alt' => 'test
 *    'strlen' => (int), // 链接名称超过此长度，则截断
 *    'external' => (bool), // 是否为外部链接。若不传值，也将判断 $path 是否为外部链接
 *    'fragment' => (string), // 锚点
 *    'query' => (array or string), // 数组：array('op' => 'insert', 'id' => 1)， 或字符串：op=insert&id=1
 *    'absolute' => (bool), // 若为 true，则输出完整路径(包含域名)
 *    'file' => (bool), // 是否为物理文件
 *    'access' => (bool), // 是否检查权限
 *  )
 * @return (string) html a，如：<a href="url">test</a>
 */
function l($text, $path, $options = array()) {
  global $current_menu_data;

  if (empty($options['attributes'])) $options['attributes'] = array();

  if (!empty($options['title'])) $options['attributes']['title'] = $text;

  if (empty($options['attributes']['title'])) {
    $options['attributes']['title'] = $text;
  }

  if ($options['strlen'] && mb_strlen($text) > $options['strlen']) {
    $text = mb_substr($text, 0, intval($options['strlen']));
  }
  
  if ($_GET['q'] == $path || $_GET['__q'] == $path) {
    $options['attributes']['class'] .= $options['attributes']['class'] ? ' active' : 'active';
  }

  if (isset($options['attributes']['title']) && strpos($options['attributes']['title'], '<') !== FALSE) {
    $options['attributes']['title'] = strip_tags($options['attributes']['title']);
  }

  if ($url = url($path, $options)) {
    return '<a href="'. $url .'"'. dd_attributes($options['attributes']) .'>'. (!empty($options['html']) ? $text : strip_tags($text)) .'</a>';
  }
}

/**
 * 输出图片标签
 * @param (string) $path
 *  图片 url，相对或绝对地址皆可
 * @param (string) $alt
 *  alt 属性
 * @param (string) $title
 *  title 属性
 * @param (array) $attributes
 *  其它属性
 * @return (string) html img，如：<img src="test.jpg" />
 */

function img($path, $alt = '', $title = '', $attributes = NULL) {
  $url = _l_external($path) ? $path : f($path);
  return '<img src="'. $url .'" alt="'. strip_tags($alt) .'" title="'. strip_tags($title) .'" ' . dd_attributes($attributes) .' />';
}

/**
 * 检查是否为绝对路径
 * @param (string) $path
 *  待检查路径
 * @return (bool)
 */
function _l_external($path) {

  // 若以 / 开头，也做为绝对路径处理，如： /user/login
  if (substr($path, 0, 1) == '/') {
    return true;
  }

  if (strpos($path, ':') !== false) {
    return in_array(current(explode(':', $path, 2)), array('http', 'https', 'ftp', 'news', 'nntp', 'telnet', 'mailto', 'irc', 'ssh', 'sftp', 'webcal', 'rtsp'));
  }
}

/**
 * 输出物理文件可访问 url
 * @param (string) $filepath
 *  文件路径
 * @return (string)
 */
function f($filepath, $absolute = false) {
  $path = '';

  if ($absolute) $path = $GLOBALS['base_url'];
  
  $path .= $GLOBALS['base_path'] . $filepath;

  return $path;
}

/**
 * 用于生成 url，不必考虑简洁链接、相对或绝对、站内或站外等因素，均可以正确判断和处理
 * @param (string) $path
 *  系统路径或外部链接
 * @param (array) $options
 *  其它参数：
 *  array(
 *    'external' => (bool), // 是否为外部链接。若不传值，也将判断 $path 是否为外部链接
 *    'fragment' => (string), // 锚点
 *    'query' => (array or string), // 数组：array('op' => 'insert', 'id' => 1)， 或字符串：op=insert&id=1
 *    'absolute' => (bool), // 若为 true，则输出完整路径(包含域名)
 *    'file' => (bool), // 是否为物理文件
 *    'access' => (bool), // 是否检查权限
 *  )
 * @return (string) 返回处理后的 url
 */
function url($path = NULL, $options = array()) {

  if (!empty($options['access']) && menu_get_item($path) != MENU_ACCESS_OK) {
    return false;
  }

  if ($options['file']) {
    return f($path, $options['absolute']);
  }

  if (!isset($options['external'])) {
    $options['external'] = _l_external($path);
  }

  if (!empty($options['external'])) {
    return menu_xss_filter($path);
  }

  if ($options['fragment']) {
    $options['fragment'] = '#'. $options['fragment'];
  }

  global $conf, $base_url, $base_path;

  /**
   * $options['query'] = array('id' => 123, 'title' => 'test');
   * $options['query'] = 'id=123&title=test';
   * $options['query'] = '123';
   */
  if (is_array($options['query'])) {
    $options['query'] = dd_query_string_encode($options['query']);
  } else if (empty($options['query'])) {
    $options['query'] = '';
  }

  if ($conf['clean_url']) {
    $url = $base_path . $path;
    if ($options['query']) {
      if (strpos($options['query'], '?') === false) {
        $url .= '?' . $options['query'];
      } else {
        $url .= '&' . $options['query'];
      }
    }
    $url .= $options['fragment'];
  } else {
    if (!empty($path)) {
      $path = '?q='. $path;
    }
    if ($options['query']) {
      $path = $path .'&'. $options['query'];
    }
    $url = $base_path . $path . $options['fragment'];
  }

  $url = menu_xss_filter($url);
  
  if (!$options['absolute']) {
    return $url;
  } else {
    return $GLOBALS['base_root'] . $url;
  }
}

/**
 * 将数组解析为字符串，以半角空格分隔，多用于 html 元素属性解析
 * @param (array) $attributes
 *  待解析数组，如：array('alt' => 'test', 'title' => 'title'
 * @return (string) 如：alt="test" title="title"
 */
function dd_attributes($attributes) {
  if (is_array($attributes)) {
    $t = '';
    foreach ($attributes as $key => $value) {
      if (isset($value)) $t .= " $key=".'"'. check_plain($value) .'"';
    }
    return $t;
  }
}

/**
 * 日志记录
 * @param (string) $type
 *  类型，自定义
 * @param (string) $value
 *  内容
 * @param (string) $link
 *  发生地址，默认为当前地址
 * @param (int) $status
 *  状态，0、普通，1、警告，2、严重
 * @param (int) $uid
 *  用户 id，默认为当前用户
 */
function dd_log($type, $value, $ext_id = NULL, $link = NULL, $status = 0, $uid = NULL) {
  if ($value) {
    if (!isset($uid)) $uid = $GLOBALS['user']->uid;
    if (!$status) $status = 0;
    if (!isset($ext_id)) $ext_id = 0;
    if (!$ip = ip_address()) $ip = '127.0.0.1';
    if (empty($link)) $link = dd_get_referer($_SERVER['REQUEST_URI']);

    db_exec('INSERT INTO {logs} (uid, ext_id, type, status, value, url, timestamp, host)
    VALUES (?, ?, ?, ?, ?, ?, ?, ?)', array($uid, $ext_id, $type, $status, $value, 
    $link, $_SERVER['REQUEST_TIME'], $ip));
  }
}

/**
 * 系统日志状态
 * @param (int) $id
 * @return (string)
 */
function dd_logs_get_status($id = NULL){
  $a = array(t('system', '普通'), t('system', '警告'), t('system', '严重'));
  return isset($id) ? $a[$id] : $a;
}

/**
 * 写入配置，若存在则更新。初始化时将加载所有配置。
 * @param $name = String
 *  配置名称，不应大于 255 个字符
 * @param $value = *
 *  配置内容，序列化存储
 * $return 无返回值，但将即时更新全局变量 $conf
 * @param $clear = 1;
 *  清除配置文件缓存
 * @param (bool) $replace
 *  是否使用 db_replace 写入数
 */
function var_set($name, $value, $clear = 1, $replace = 1) {
  global $conf;

  $v = array('name' => $name);

  if ($replace) {
    $v['value'] = $value;
    if (!db_replace('variable', $v)) {
      return;
    }
  } else {
    $v['value'] = serialize($value);
    db_exec('DELETE FROM {variable} WHERE name = ?', array($name));
    db_exec('INSERT INTO {variable} (name, value) VALUES (?, ?)', array($name, serialize($value)));
  }

  $conf[$name] = $value;
  if ($clear) var_init();
}

/**
 * 写入自定义数据
 * @param (string) $name
 *  名称
 * @param (*) $value
 *  数据
 * @return (bool)
 */
function custom_set($name, $value) {
  cache_del('cid', 'ctm_' . $name);
  return db_replace('custom', array('name' => $name, 'value' => $value));
}

/**
 * 读取配置
 * @param (string) $name
 *  配置名称
 * @param (*) $default
 *  默认值，若没有请求的配置，则返回此值
 * @return (*);
 */
function var_get($name, $default = NULL) {
  global $conf;
  return isset($conf[$name]) ? $conf[$name] : $default;
}

/**
 * 读取自定义数据
 * @param (string) $name
 *  名称
 * @param (*) $default
 *  默认值，若没有请求的配置，则返回此值
 * @return (*);
 */
function custom_get($name, $default = NULL) {
  static $custom;
  if (!isset($custom[$name])) {
    if ($cache = cache_get('ctm_' . $name)) {
      $custom[$name] = $cache->data;
    } else {
      if ($value = db_query('SELECT value FROM {custom} WHERE name = ?',
      array($name), array('return' => 'column'))) {
        $custom[$name] = unserialize($value);
      } else {
        $custom[$name] = $default;
      }
      cache_set('ctm_' . $name, $custom[$name]);
    }
  }
  return $custom[$name];
}

/**
 * 读取子配置，针对数组配置，参数同 var_get()
 * @param (string) $name
 *  配置名称
 * @param (string) $key
 *  子配置名称，即数组键名
 * @param (*) $default
 *  默认值，若没有请求的配置，则返回此值
 * @return (*);
 */
function var_get_key($name, $key, $default = false) {
  global $conf;
  return isset($conf[$name][$key]) ? $conf[$name][$key] : $default;
}

/**
 * 删除配置
 * @param (string) $name
 *  配置名称
 * @param (bool) $clear
 *  是否立即刷新配置缓存
 */
function var_del($name, $clear = true) {
  global $conf;
  db_exec('DELETE FROM {variable} WHERE name = ?', array($name));
  unset($conf[$name]);
  if ($clear) {
    var_init();
  }
}

/**
 * 删除自定义数据
 * @param (string) $name
 *  名称
 */
function custom_del($name) {
  db_exec('DELETE FROM {custom} WHERE name = ?', array($name));
}

/**
 * 更新配置缓存，无参数，无返回值
 */
function var_init() {
  global $conf;
  if ($fetch = db_query('SELECT * FROM {variable}')) {
    foreach ($fetch as $variable) {
      $conf[$variable->name] = unserialize($variable->value);
    }
  }
  cache_system_set_file('conf.php', 'conf', $conf);
}

/**
 * 多语言
 * 
 * Dida 的多语言机制较为灵活，模块开发者可以使用自己熟悉的语言，并不强制使用中文或英文，在 module.info 设置语言即可
 * 在代码中使用 t() 函数实现多语言，例如：t('user', '登录') 第一个参数为模块名称
 *
 * 有些情况下，需要实现多语言的内容是管理员在设置中输入的，模块开发者预先并不知道输入内容和语言
 * 为这种情况，在全局设置增加了管理语言，管理员可设置该选项，系统默认所有动态字符串，均为所设置的语言
 * 模块可提供输入表单，将管理员的输入保存在数据库中，同时模块在输出时，第一个参数使用 NULL，例如：t(NULL, $text) 即可
 * 此时，不管 $text 的语言是什么，均视作管理语言(admin_language)
 *
 * @param string $module
 *  默认系统名称，赋值则优先使用该模块定义的值
 * @param string $index
 *  语言字符串键名，若无对应值，则返回键名
 * @param array $args
 *  要转换的字符串参数
 * @param string $language
 *  指定语言
 * 所有模块的语言文件缓存在 {$conf_dir}/cache/{$language}.php
 * 可在相同目录自定义语言翻译文件，命名为 custom_{$language}.php，自定义翻译，优先级最高
 */
function t($module, $index, $args = array(), $language = NULL) {
  static $lang;
  global $language_code, $conf;

  if (empty($language)) $language = $language_code;
  
  /**
   * 1、$module 为空，需要加载翻译文件并翻译
   * 2、请求的语言 $language 与模块 ($module) info 中指定的界面语言不相同，需要加载翻译文件并翻译
   */ 
  if ((!empty($module) && !empty($conf['module_theme'][$module]) && $language != $conf['module_theme'][$module]['language']) || (empty($module) && $conf['admin_language'] != $language)) {
    if (!isset($lang[$language])) {

      $lang_file = DIDA_ROOT . '/' . $GLOBALS['conf_dir'] . '/cache/languages/';

      if (is_file($lang_file . $language . '.php')) {
      
        include $lang_file . $language . '.php';

        $lang[$language] = $l;
        $_j = $j;

        $j = NULL;
        // 加载自定义翻译文件
        if (is_file($lang_file . 'custom_' . $language . '.php')) {
          $l = array();
          include $lang_file . 'custom_' . $language . '.php';
          $lang[$language] = array_merge($lang[$language], $l);
          if (!empty($j)) $_j = array_merge($_j, $j);
        }
        
        // 输出 js 文件中使用到的翻译字符串
        if ($language_code == $language && $_j) {
          dd_add_js('Dida.locale = ' . dd_to_js($_j), 'inline_nojq', 0, 'footer');
        }
        
      } else {
        $lang[$language] = false;
      }
    }

    if (!empty($lang[$language]) && !empty($lang[$language][$index])) {
      if (!empty($module) && !empty($lang[$language]['__' . $module]) && !empty($lang[$language]['__' . $module][$index])) {
        $index = $lang[$language]['__' . $module][$index];
      } else {
        $index = $lang[$language][$index];
      }
    }

  }
  
  if (empty($args)) {
    return $index;
  } else {
    foreach ($args as $key => $value) {
      switch ($key[0]) {
        case '@':
          // 格式化
          $args[$key] = check_plain($value);
        break;
        case '%':
          // 替换占位符 <em> </em
          $args[$key] = '<em>'. $value . '</em>';
        break;
          //case '!': default:
          // 其它不做处理
      }
    }
    return strtr($index, $args);
  }
}

/**
 * 合并模块语言文件
 * @param (array) $lists
 *  模块列表，默认为所有模块文件夹下的语言文件，modules/test/lang/*
 * @param (array) $language
 *  需导入的语言，默认为所有设定的语言
 * 语言文件缓存在 ./sties/cache/lang_{$language}.lang，合并规则：
 *  若有相同键名，取第一个保存为默认字符串，其它保存为特殊翻译串。
 * 特殊翻译保存在以模块系统名称加双下划线为键名的数组下
 * 如 __block，以此避免与模块名称翻译产生冲突。
 *  以此增加语言字符串重用率，减少语言文件体积。同时可为相同字符串提供多种翻译。
 * 示例：
 *  system.module 定义了 $lang['cache'] = '缓存'
 *  block.module 定义了 $lang['cache'] = '区块缓存'; // 假设
 *  在导入合并时，'缓存' 将作为 'cache' 的默认翻译，而 '区块缓存' 作为特殊翻译
 * 最终的语言文件如：
 *  $lang = array(
 'cache' => '缓存',
 '__block' => array(
 'cache' => '区块缓存',
 )
 )
 */
function dd_set_lang($lists = NULL, $language = array()) {
  global $conf;

  if (empty($lists)) {
    if ($conf['module_theme']) {
      foreach ($conf['module_theme'] as $module => $info) {
        $lists[$module] = $info['path'];
      }
    }
  }
  
  if (empty($language)) {
    if ($conf['languages']) {
      foreach ($conf['languages'] as $key => $name) {
        $language[] = $key;
      }
    } else {
      $language[] = 'zh-hans';
    }
  }

  if ($lists) {
    $cache = DIDA_ROOT . '/' . $GLOBALS['conf_dir'] . '/cache/languages';

    if (!is_dir($cache)) {
      dd_set_message(t('system', '%dir 不存在，无法更新语言文件', array('%dir' => $cache)), 'error');
      return false;
    }

    foreach ($language as $code) {
      // 所有翻译字符串
      $l = array();
      // 在 js 文件中使用的字符串
      $j = array();
       	
      foreach ($lists as $module => $path) {
        $lang = $js_lang = array();

        $path = DIDA_ROOT . '/' .  $path . '/languages/' . $code . '.php';

        if (!is_file($path)) continue;

        include $path;
        
        if (empty($lang) || !is_array($lang)) continue;

        if (!empty($js_lang)) $j += $js_lang;

        $newLang = array_diff_key($lang, $l);
        
        $reLang = array_intersect_key($lang, $l);

        if ($newLang) {
          $l = array_merge($l, $newLang);
        }

        if (empty($reLang)) continue;
        
        foreach ($reLang as $rekey => $resub) {
          if ($l[$rekey] == $resub) {
            unset($reLang[$rekey]);
          }
        }

        if ($reLang) {
          $moduleKey = '__' . $module;
          if (empty($l[$moduleKey])) {
            $l[$moduleKey] = $reLang;
          } else {
            $l[$moduleKey] = array_merge($reLang, $l[$moduleKey]);
          }
        }

      }
      
      if ($l) {
        if (!empty($j)) {
          foreach ($j as $key => $_v) {
            $j[$key] = $l[$key];
          }
        } 
        // 合并为翻译缓存文件
        $output = '<?php';
        $output .= "\n";

        $message = "\n\n/**\n";
        $message .= ' * 翻译语言：' . $conf['languages'][$code] . '(' . $code . ")\n";
        $message .= ' * 共有 ' . count($l) . " 条翻译\n";
        $message .= " */\n\n";

        $output .= '$l = ' . var_export($l, true) . ";\n";
        $output .= '$j = ' . var_export($j, true) . ";\n";

        $path =  DIDA_ROOT . '/'  . $GLOBALS['conf_dir'] . '/cache/languages/' . $code . '.php';
        
        if (file_put_contents($path, $output)) {
          @chmod($path, 0777);
          dd_set_message(t('system', '%name 导入成功', array('%name' => $code)));
        } else {
          dd_set_message(t('system', '%name 导入失败，请检查缓存目录权限', array('%name' => $code)), 'error');
        }
      }
    }
  }
}

/**
 * 调用 hook_token_replace 进行遍历替换
 * @param (string) $text
 *  要替换字符串
 * @param (*) $value
 *  包含一组替换值
 * @param (array) $module
 *  指定模块，若不指定，则调用所有模块的 hook_token_replac
 */
function dd_get_token($text, $value = NULL, $modules = NULL) {
  if (!is_array($modules)) {
    $hooks = dd_get_hook('token_replace');
  } else {
    foreach ($modules as $module) {
      $hooks[$module] = $module . '_token_replace';
    }
  }

  if ($hooks) {
    foreach ($hooks as $function) {
      $text = $function($text, $value);
    }
  }
  return $text;
}

/**
 * hook 缓存，将 hook 函数缓
 */
function dd_get_hook($hook, $clear = 0) {
  static $hooks;
  if (!isset($hooks[$hook])) {
    if (!$clear && ($cache = cache_get('hook_cache'))) {
      $hooks = $cache->data;
    }
    if (!$cache->data || !is_array($hooks[$hook])) {
      $hooks[$hook] = array();
      foreach ($GLOBALS['conf']['modules'] as $module => $info) {
        $function = $module .'_'. $hook;
        if (function_exists($function)) {
          $hooks[$hook][$module] = $function;
        }
      }
      cache_set('hook_cache', $hooks);
    }
  }
  return $hooks[$hook];
}

/**
 * 获取随机字符串
 */
function dd_rand_str() {
  return md5(mt_rand(). time() . session_id());
}

/**
 * 获取指定长度的随机字符串
 * @param (int) $len
 *  字符串长度
 * @param (string) $query
 *  查询数据库语句，若有此值得，则获取不重复的值
 *  验证是否已使用，如：SELECT COUNT(name) FROM {users} WHERE name = ?
 * @return (string)
 */
function dd_get_rand_string($len, $query = NULL) {
  $chars = array(
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
  'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G',  
  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
  'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2',  
  '3', '4', '5', '6', '7', '8', '9');
  
  $charsLen = count($chars) - 1;
  
  $t = 1;
  while ($t) {
    
    $output = '';
    for ($i = 0; $i < $len; $i++)  {
      $output .= $chars[mt_rand(0, $charsLen)];
    }
    
    if ($query) {
      if (!db_query($query, array($output), array('return' => 'colum'))) {
        $t = 0;
      }
    } else {
      $t = 0;
    }
    
  }
  
  return $output;
}

/**
 * 获取客户端 ip
 */
function ip_address() {
  static $ip_address = NULL;
  if (!isset($ip_address)) {
    if (isset($_SERVER['REMOTE_ADDR'])) {
      $ip_address = $_SERVER['REMOTE_ADDR'];
    }
    if (array_key_exists('HTTP_X_FORWARDED_FOR', $_SERVER)) {
      $ip_address = array_pop(explode(',', $_SERVER['HTTP_X_FORWARDED_FOR']));
    }
  }
  return $ip_address;
}

/**
 *  用户验证
 */
function dd_user_validate() {
  global $user;

  if (empty($user)) $user = new stdClass();
  
  if (!empty($_COOKIE[session_name()]) && !empty($_SESSION['user'])) {
    
    $user = $_SESSION['user'];
    $active = dd_get_cookie('time');
    $time = time();
    dd_set_cookie('time', $time);
    
    if ($_SESSION['user']->session == $_COOKIE[session_name()]) {
 
      if (empty($user->__SETVALIDATE)) {
        // 每 600 秒验证一次 session。如上次活动时间为 0，则强行验证一次 session
        if (empty($active) || ($active > ($user->active + 600))) {
          if (!db_query('SELECT uid FROM {users} WHERE uid = ? AND session = ?',
          array($user->uid, $_COOKIE[session_name()]), array('return' => 'column'))) {
            user_logout();  // 游客
            dd_set_message(t('system', '登录超时，请重新登录'), 'error');
          } else {
            $user->active = $time;
          }
        }
      }
     
      if ($active > ($time - 1800)) {
        // 两次活动时间间隔 30 分钟内
        if (($active > ($user->active + 300)) && ($active < ($user->active + 600))) {
          //每300秒写入一次用户在线时间。若距上次活动超过600秒，为不活动用户。
          $online = $active - $user->active; //增加在线时间
          db_exec('UPDATE {users} SET active = ?, online = online + ? WHERE uid = ?', array($time, $online, $user->uid));
          $user->active = $time;
          $user->exp += $exp;
          $user->online += $online;
          $_SESSION['user'] = $user;
        }
      }
    }
  }
 
  if (empty($user->language)) {
    $user->language = $GLOBALS['language_code'];
  }

  if (empty($user->uid)) {
    $user = user_anonymous($user);  // 游客
  }

  // 默认时区
  if (empty($user->timezone)) {
    $user->timezone = var_get('default_timezone', 'Asia/Chongqing');
  }

  if ($user->timezone != 'Asia/Chongqing') {
    date_default_timezone_set($user->timezone);
  }

  // 管理员强制要求某个用户退出
  if ($user->uid && !empty($GLOBALS['conf']['dida_user_enforce_logout'])) {
    $key = array_search($user->uid, $GLOBALS['conf']['dida_user_enforce_logout']);
    if ($key !== false) {
      unset($GLOBALS['conf']['dida_user_enforce_logout'][$key]);
      var_set('dida_user_enforce_logout', $GLOBALS['conf']['dida_user_enforce_logout']);

      // 强行退出
      $user = user_logout();
      $user = user_anonymous();
    }
  }
}

/**
 * 将字符串简单加密，并不能保证安全性，仅比明文好一点
 * @param (string) $text
 *  待加密的字符串
 * @param (string) $key
 *  密钥，解密时，必须指定密钥，才可正确获取数据
 * @return (string) 加密后的字符串
 *
 * @demo：
 *  加密：dd_encrypt($text, 'key');
 *  解密：dd_decrypt($text, 'key');
 */
function dd_encrypt($text, $key = NULL) {
  $new = array();
  $key = dd_encrypt_key($key);
  $text = substr($key['md5'], 16, 8) . $text;

  for ($i=0; $i <= strlen($text); $i++ ) {
    $new[$i] = ord($text[$i]) * $key['ascii'];
  }

  $new = implode ('.', $new);
  return base64_encode($new);
}

/**
 * 字符串解密
 * @param (string) $text
 *  待解密的字符串
 * @param (string) $key
 *  密钥，与加密时的一致
 * @return (string) 解密后的字符串
 */
function dd_decrypt($text, $key = NULL) {
  $new = array();

  $text = base64_decode($text);
  $array = explode('.', $text);
  $key = dd_encrypt_key($key);
  $count = count($array);

  for ($i = 0; $i < $count; $i++) {
    if ($array[$i]) {
      $new[$i] = chr($array[$i] / $key['ascii']);
    }
  }

  $new = implode('', $new);

  if (substr($key['md5'], 16, 8) == substr($new, 0, 8)) {
    return substr($new, 8);
  } else {
    return false;
  }
}

/**
 * 获取密钥的 ASCII 值
 * @param (string) $key
 *  若不指定，则使用默认密钥
 * @return (array) 密钥的 ASCII值和md5值，如：array('md5' => '', 'ascii' => '');
 */
function dd_encrypt_key($key = NULL) {
  if (!isset($key)) {
    $key = var_get('crypt_default_key', $GLOBALS['cookie_domain']);
  }

  static $datas;
  if (!isset($datas[$key])) {
    $datas[$key]['md5'] = md5($key);
    $datas[$key]['ascii'] = 0;

    for ($i = 0; $i <= strlen($datas[$key]['md5']); $i++ ) {
      $datas[$key]['ascii'] += ord($datas[$key]['md5'][$i]);
    }
  }
  return $datas[$key];
}

/**
 * 验证超级管理员
 */
function dd_is_root_admin() {
  return $GLOBALS['user']->uid == 1;
}

/**
 * 注销，触发 hook_user_logout($ac)
 */
function user_logout() {
  global $user, $cookie_domain;

  dd_log('user_logout', t('system', '用户退出'), 0, dd_get_referer($_SERVER['HTTP_REFERER']), 0, $user->uid);

  module_invoke_all('user_logout', $user);

  setcookie(session_name(), '', -1, '/');
  session_destroy();
  $user = user_anonymous();
  dd_set_message(t('system', '退出成功'));

  dd_goto('');
}

/**
 * 获取站内来路
 * @param (string) $url
 */
function dd_get_referer($url) {
  static $data;

  if (!isset($data[$url])) {
    if (substr($url, 0, 1) == '/') {
      $a = $GLOBALS['base_path'];
    } else if(substr($url, 0, 4) == 'http'){
      $a = $GLOBALS['base_url'].'/';
    }

    $strlen = strlen($a);

    if (substr($url, 0, $strlen) == $a) {
      $data[$url] = substr($url, $strlen);
      if (!$data[$url]) {
        $data[$url] = '<front>';
      }
    } else {
      $data[$url] = $url;
    }
  }

  return $data[$url];
}

/**
 * 游客信息
 */
function user_anonymous($data = NULL) {
  global $cookie_domain, $conf;

  $user = new stdClass();

  setcookie('time', '', -1, '/', $cookie_domain);
 
  if (!empty($data)) {
    foreach ($data as $key => $value) {
      $user->{$key} = $value;
    }
  }

  $user->uid = 0;
  $user->name = $user->roles[var_get_key('user', 'anonymous_role', 1)] = var_get_key('user', 'anonymous', '游客');
  $user->session = $_COOKIE[session_name()];
  $user->host = ip_address();
 
  return $user;
}

/**
 * 获取域名、路径、cookie 作用域、session id
 */
function conf_init() {
  global $cookie_domain, $base_url, $base_root, $base_path, $is_front, $http_host;

  $is_front = dd_is_front();

  $base_root = (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] == 'on') ? 'https' : 'http';
  $base_url = $base_root .= '://'. $_SERVER['HTTP_HOST'];
  $http_host = $_SERVER['HTTP_HOST'];
  if ($dir = trim(dirname($_SERVER['SCRIPT_NAME']), '\,/')) {
    $base_path = "/$dir";
    $base_url .= $base_path;
    $base_path .= '/';
  } else {
    $base_path = '/';
  }

  if (empty($cookie_domain)) {
    list( , $session_name) = explode('://', $base_url, 2);
    
    if (ini_get('session.cookie_secure')) {
      $session_name .= 'SSL';
    }
  
    if (!empty($_SERVER['HTTP_HOST'])) {
      $cookie_domain = check_plain($_SERVER['HTTP_HOST']);
    }
    $cookie_domain = ltrim($cookie_domain, '.');
    if (strpos($cookie_domain, 'www.') === 0) {
      $cookie_domain = substr($cookie_domain, 4);
    }
    $cookie_domain = explode(':', $cookie_domain);
    $cookie_domain = '.'. $cookie_domain[0];
    if (count(explode('.', $cookie_domain)) > 2 && !is_numeric(str_replace('.', '', $cookie_domain))) {
      ini_set('session.cookie_domain', $cookie_domain);
    }
  } else {
    $session_name = $cookie_domain;
    ini_set('session.cookie_domain', $cookie_domain);
  }
  
  $session_name = 'DIDA' . md5($session_name);
  
  session_name($session_name);

  if (!empty($_COOKIE[$session_name])) {
    session_id($_COOKIE[$session_name]);
  }
  
  session_start();

  if (dd_get_cookie('remember')) {
    setcookie(session_name(), session_id(), -1, '/', $cookie_domain);
    dd_set_cookie('remember', 0);
  }
}

/**
 * 写入 cookie，同时加密待写入的数据
 * @param (string) $name
 *  cookie 名称
 * @param (string) $value
 *  待写入的值
 * @param (int) $expires
 *  过期时间
 * @param (string) $path
 *  作用路径
 * @param (string) $domain
 *  作用域
 */
function dd_set_cookie($name, $value, $expires = NULL, $path = '/', $domain = NULL) {
  if (!isset($expires)) {
    $expires = $_SERVER['REQUEST_TIME'] + 31536000;
  }

  if (!isset($domain)) {
    $domain = $GLOBALS['cookie_domain'];
  }

  setcookie($name, dd_encrypt($value), $expires, $path, $domain);
}

/**
 * 获取通过 dd_set_cookie 加密写入的 cookie
 * @param (string) $name
 *  cookie 名称
 * @return (string)
 */
function dd_get_cookie($name) {
  if (isset($_COOKIE[$name])) {
    return dd_decrypt($_COOKIE[$name]);
  }
}

/**
 * 转至安装文件
 */
function dd_to_install() {
  conf_init();
  header('Location: ' . $GLOBALS['base_path'] . 'install.php');
  exit;
}

/**
 * 获得当前毫秒数
 */
function dd_get_microtime() {
  list($usec, $sec) = explode(" ", microtime());
  $time = ((float)$usec + (float)$sec);
  if (function_exists('bcmul')) {
    return bcmul($time, 1000);
  } else {
    return $time;
  }
}

/**
 * 获取指定时间的星期的中文表达
 * @param (int) $timestamp
 *  时间戳
 * @return (string)
 */
function format_date_week_cn($timestamp) {
  $cn = array(
    t('system', '星期日'), t('system', '星期一'), t('system', '星期二'),
    t('system', '星期三'), t('system', '星期四'), t('system', '星期五'), t('system', '星期六')
  );
  return $cn[date('w', $timestamp)];
}

/**
 * 格式时间戳
 * @param (int) $timestamp
 *  时间戳
 * @param (string) $type
 *  处理方式
 * @param (string) $format
 *  自定义格式
 * @return (string) 格式化之后的时间
 */
function format_date($timestamp, $type = 'default', $format = '') {
  switch ($type) {
    case 'small':
      $format = 'Y/m/d';
      break;
    case 'large':
      $format = 'Y/m/d H:i:s ' . format_date_week_cn($timestamp);
      break;
    case 'custom':
      if (!$format) $format = 'Y/m/d H:i:s';
      break;
    case 'medium':
      $format = 'Y/m/d H:i:s';
      break;
    default:
      $format = 'Y/m/d H:i:s';
  }
  return date($format, $timestamp);
}

/**
 * 计算给定的秒数距离当前时间多久
 * @param int $timestamp
 *  秒数，通常是当前时间减去另一个时间
 * @param int $granularity
 *  显示层级，默认为 年 周 天 小时 分钟 秒
 * @return string
 */
function format_interval($timestamp, $granularity = 5) {
  $units = array(
    31536000 => t('system', '年'), 
    604800 => t('system', '周'), 
    86400 => t('system', '天'),
    3600 => t('system', '小时'),
    60 => t('system', '分钟'),
    1 => t('system', '秒')
  );

  $output = '';

  foreach ($units as $key => $value) {
    if ($timestamp >= $key) {
      $output .= floor($timestamp / $key) . $value;
      $timestamp %= $key;
      $granularity--;
    }
    if ($granularity == 0) {
      break;
    }
  }

  return $output ? $output : t('system', '0 秒');
}

/**
 * 将字符串形式的日期转时间戳，日期格式：2010-05-25 或 2010/05/25 或 2010-05-25 12:24:32
 * @param (string) $text
 *  待转换日期
 * @return (int) 时间戳期
 */
function format_date_mktime($text) {
  static $data;
  if (!isset($data[$text])) {
    $data[$text] = '';

    if (strpos($text, '-') !== false) {
      $t = '-';
    } else if (strpos($text, '/') !== false) {
      $t = '/';
    } else {
      $t = NULL;
    }

    $time = array(0, 0, 0);

    if (strpos($text, ':') !== false) {
      if ($a = end(explode(' ', $text, 2))) {
        $time = explode(':', $a);
      }
    }

    if ($t) {
      $d = explode($t, $text);
      if (count($d) == 3) {
        $data[$text] = mktime($time[0], $time[1], $time[2], $d[1], $d[2], $d[0]);
      }
    }
  }
  return $data[$text];
}

/**
 * 获取当天零点的时间戳
 * @return (string)
 */
function format_get_current() {
  static $time;
  if (!isset($time)) {
    $time = format_date_mktime(format_date(time(), 'custom', 'Y-m-d'));
  }
  return $time;
}

/**
 * 处理 PHP 错误信息
 */
function dd_php_error($errno, $message, $filename, $line = NULL, $context = NULL) {
  if (error_reporting() == 0) return;
  if ($errno & (E_ALL ^ E_NOTICE)) {
    $msg = t('system', '!message。文件：%file(%line 行)', array('!message' => $message, '%file' => $filename, '%line' => $line));

    text_log('php_error', dd_error_msg($msg));

    global $user, $conf;
    if (empty($conf['debug'])) return;
    if ($conf['debug'] == 2 && (empty($user->uid) || $user->uid != 1)) return;
    dd_set_message($msg, 'error');

  }
}

/**
 * 以 xmprpc 方式通知服务器内容更新
 * @param (array) $data
 *  数据
 * @param (array) $service
 *  自定义通知服务器列表
 * @param (string) $method
 *  应答方式
 * @return (array) 接口响应
 */
function dd_set_ping($data, $service = array(), $method = 'weblogUpdates.extendedPing') {

  if (!$service) {
    $service = array(
      'http://ping.baidu.com/ping/RPC2',
      'http://rpc.pingomatic.com/',
      'http://blog.yodao.com/ping/RPC2',
      'http://rpc.technorati.com/rpc/ping',
      'http://blogsearch.google.com/ping/RPC2' // google
    );
  }

  $xml = '<?xml version="1.0"?>';
  $xml .= '<methodCall><methodName>'.$method.'</methodName>';
  $xml .= '<params>';


  foreach ($data['params'] as $param) {
    $xml .= '<param><value>'.$param.'</value></param>';
  }

  $xml .= '</params>';
  $xml .= '</methodCall>';

  foreach ($service as $ping) {
    $result[] = dd_http_request($ping, $xml, 'POST', array('Content-Type' => 'text/xml'));
  }

  return $result;
}

/**
 * 执行一个 HTTP 请求
 *
 * @param (string) $url
 *  合法完整的 URL
 * @param (array) $headers
 *  HTTP header 数组
 * @param (string) $method
 *  请求方式：POST、GET、PUT
 * @param (*) $data
 *  写入的数据，字符串或数组、对象
 * @param (int) $retry
 *  重定向次数
 * @param (float) $timeout
 *  超时时间，默认 15 秒
 * @param (array) $opt
 *  其它配置参数
 * @return
 *  返回一个包含执行结果的对象
 *  $result->error：错误提示
 *  $result->code：HTTP 响应代码，小于 0 则为请求失效
 *  $result->data：返回内容
 */
function dd_http_request($url, $data = NULL, $method = 'POST', $headers = array(), $retry = 3, $timeout = 15.0, array $opt = array()) {
  $result = new stdClass();

  $uri = parse_url($url);

  if ($uri == FALSE) {
    $result->error = t('system', '无法解析网址');
    $result->code = -1001;
    return $result;
  }

  if (!isset($uri['scheme'])) {
    $result->error = t('system', '无效的协议');
    $result->code = -1002;
    return $result;
  }

  $options = array(
    'headers' => empty($headers) ? array() : $headers,
    'method' => $method,
    'data' => NULL,
    'history' => array(),
    'max_redirects' => $retry,
    'timeout' => (float) $timeout,
    'context' => NULL,
  );

  if (!empty($opt)) {
    $options = array_merge($options, $opt);
  }

  switch ($uri['scheme']) {
    case 'http': case 'feed':
      $port = isset($uri['port']) ? $uri['port'] : 80;
      $socket = 'tcp://' . $uri['host'] . ':' . $port;
      $options['headers']['Host'] = $uri['host'] . ($port != 80 ? ':' . $port : '');
    break;
    case 'https':
      // 须编译 openssl
      $port = isset($uri['port']) ? $uri['port'] : 443;
      $socket = 'ssl://' . $uri['host'] . ':' . $port;
      $options['headers']['Host'] = $uri['host'] . ($port != 443 ? ':' . $port : '');
    break;
    default:
      $result->error = t('system','%string 协议不存在', array('%string' => $uri['scheme']));
      $result->code = -1003;
      return $result;
  }

  // 使用代理
  if (!empty($options['proxy'])) {
    $socket = $options['proxy'];
  }

  if (empty($options['context'])) {
    $fp = @stream_socket_client($socket, $errno, $errstr, $options['timeout']);
  } else {
    $fp = @stream_socket_client($socket, $errno, $errstr, $options['timeout'], STREAM_CLIENT_CONNECT, $options['context']);
  }
  
  // 服务器无响应
  if (!$fp) {
    $result->code = -$errno;
    $result->error = trim($errstr);
    return $result;
  }

  if (empty($options['headers']['User-Agent'])) {
    $options['headers']['User-Agent'] = 'Mozilla/5.0 (Windows; U; Windows NT 6.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 GTBDFff GTB7.0';
  }
  
  if (is_array($data) || is_object($data)) {
    $options['data'] = '';
    foreach ($data as $key => $value) {
      if (!is_array($value)) {
        $options['data'] .= rawurlencode(trim($key)) .'=' . rawurlencode(trim($value)) . '&';
      } else {
        foreach ($value as $_k => $_v) {
          $options['data'] .= rawurlencode(trim($key)) . '[' . $_k . ']' .'=' . rawurlencode(trim($_v)) . '&';
        }
      }
    }
    $options['data'] = substr($options['data'], 0, -1);
  } else {
    $options['data'] = $data;
  }

  // GET 方式，将参数附加在 url 之后
  if (!empty($options['data']) && $method == 'GET') {
    if (isset($uri['query'])) {
      $uri['query'] .= '&' .$options['data'];
    } else {
      $uri['query'] = $options['data'];
    }
    unset($options['data']);
  }

  // 解析路径
  $path = isset($uri['path']) ? $uri['path'] : '/';
  if (isset($uri['query'])) {
    $path .= '?' . $uri['query'];
  }
  
  if (isset($options['data']) && ($method == 'POST' || $method == 'PUT')) {
    $options['headers']['Content-Length'] = strlen($options['data']);
  }

  // 身份验证
  if (isset($uri['user'])) {
    $options['headers']['Authorization'] = 'Basic ' . base64_encode($uri['user'] . (!empty($uri['pass']) ? ":" . $uri['pass'] : ''));
  }

  if ($method == 'POST' && empty($headers['Content-Type'])) {
    $options['headers']['Content-Type'] = 'application/x-www-form-urlencoded; charset=utf-8';
  }
  
  if (!empty($options['proxy'])) {
    if ($port != '80') {
      $path = $uri['scheme'] . '://' . $uri['host'] . ':' . $port . $path;
    } else {
      $path = $uri['scheme'] . '://' . $uri['host'] . $path;
    }
  }

  $request = $options['method'] . ' ' . $path . " HTTP/1.0\r\n";
  foreach ($options['headers'] as $name => $value) {
    $request .= $name . ': ' . trim($value) . "\r\n";
  }
  
  $request .= "\r\n";
  
  if (isset($options['data'])) {
    $request .= $options['data'];
  }

  $result->request = $request;
  
  // 写入数据
  stream_set_timeout($fp, floor($options['timeout']), floor(1000000 * fmod($options['timeout'], 1)));
  fwrite($fp, $request);

  // 读取响应头
  $info = stream_get_meta_data($fp);
  $alive = !$info['eof'];
  $response = '';

  while ($alive) {
    $chunk = fread($fp, 1024);
    $response .= $chunk;
    $info = stream_get_meta_data($fp);
    $alive = !$info['eof'] && $chunk;
  }
  
  fclose($fp);
  
  // 解析响应
  list($response, $result->data) = explode("\r\n\r\n", $response, 2);
 
  $response = preg_split("/\r\n|\n|\r/", $response);
  
  list($protocol, $code, $status_message) = explode(' ', trim(array_shift($response)), 3);
  $result->protocol = $protocol;
  $result->status_message = $status_message;
  
  $result->headers = array();
  $result->headers['cookies'] = array();
  
  // 响应头
  while ($line = trim(array_shift($response))) {
    list($name, $value) = explode(':', $line, 2);
    $name = strtolower($name);
    if ($name == 'set-cookie') {
      if (isset($result->headers[$name])) {
        // Cookie: 以逗号分隔取得的 Cookie
        $result->headers[$name] .= ',' . trim($value);
      } else {
        $result->headers[$name] = trim($value);
      }
      if (is_array($result->headers['cookies']) && strpos($value, ';') !== false) {
        $_c = explode(';', $value, 3);
        if (strpos($_c[0], '=') !== false) {
          list($_n, $_d) = explode('=', $_c[0], 2);
          $result->headers['cookies'][trim($_n)] = $_d;
        }
      }
    } else {
      $result->headers[$name] = trim($value);
    } 
  }

  // http 1.1 分块输出，过滤 Transfer-Encoding 标识
  // @TODO 不做处理
  //if ($result->data && !empty($result->headers['transfer-encoding'])
  //  && $result->headers['transfer-encoding'] == 'chunked') {
  //  $result->data = dd_filter_unchunkHttp11($result->data);
  //}
  
  $responses = array(
    100 => 'Continue',
    101 => 'Switching Protocols',
    200 => 'OK',
    201 => 'Created',
    202 => 'Accepted',
    203 => 'Non-Authoritative Information',
    204 => 'No Content',
    205 => 'Reset Content',
    206 => 'Partial Content',
    300 => 'Multiple Choices',
    301 => 'Moved Permanently',
    302 => 'Found',
    303 => 'See Other',
    304 => 'Not Modified',
    305 => 'Use Proxy',
    307 => 'Temporary Redirect',
    400 => 'Bad Request',
    401 => 'Unauthorized',
    402 => 'Payment Required',
    403 => 'Forbidden',
    404 => 'Not Found',
    405 => 'Method Not Allowed',
    406 => 'Not Acceptable',
    407 => 'Proxy Authentication Required',
    408 => 'Request Time-out',
    409 => 'Conflict',
    410 => 'Gone',
    411 => 'Length Required',
    412 => 'Precondition Failed',
    413 => 'Request Entity Too Large',
    414 => 'Request-URI Too Large',
    415 => 'Unsupported Media Type',
    416 => 'Requested range not satisfiable',
    417 => 'Expectation Failed',
    500 => 'Internal Server Error',
    501 => 'Not Implemented',
    502 => 'Bad Gateway',
    503 => 'Service Unavailable',
    504 => 'Gateway Time-out',
    505 => 'HTTP Version not supported',
  );

  if (!isset($responses[$code])) {
    $code = floor($code / 100) * 100;
  }
  
  $result->code = $code;

  switch ($code) {
    case 200: // 成功
    case 304: // 缓存页面
      break;
    case 301: // 永久重定向
    case 302: // 临时重定向
    case 307: // 临时重定向
      
      if (substr($result->headers['location'], 0, 4) == 'http') {
        $location = $result->headers['location'];
      } else {
        $location = $uri['scheme'] . '://' . $uri['host'] . $result->headers['location'];
      }
      
      if ($retry) {
        $options['max_redirects']--;
        $headers['Referer'] = $url;
        if (!empty($result->headers['set-cookie'])) {
          $headers['Cookie'] = $result->headers['set-cookie'];
        }
        $opt['history'] = array($result);
        $result = dd_http_request($location, $data, $method, $headers, $options['max_redirects'], $timeout, $opt);
        $result->redirect_code = $result->code;
      }
      
      $result->redirect_url = $location;
    break;
    default:
      $result->error = $status_message;
  }

  if (!empty($result->history)) {
    $result->history[] = $options['history'];
  } else {
    $result->history = $options['history'];
  }
  
  return $result;
}

/**
 * 将通过 dd_http_request 获取的 cookie 转换为与浏览器提交格式
 * 获取到的 cookie 格式一般为：
 *  name=value; path=/; expires=Tue, 29-Mar-22 18:52:18 GMT; domain=test.com,name2=......
 * 转换为浏览器提交格式：
 *  name=value;name2=value;name3=value
 * @param string $cookie 
 *  获取到的 cookie，包含了 path、domain 等
 * @access public
 * @return string
 */
function dd_http_request_cookie($cookie) {
  // 时间中一般包含有逗号(,)，在 dd_http_request 中，cookie 也以逗号分隔，为免冲突，替换掉
  $cookie = preg_replace(array('/expires=.*?;/i', '/domain=.*?;/i', '/HttpOnly/i', '/Path=.*?[,|;]/i', '/Path=\//i'), '', $cookie);

  if (strpos($cookie, ',') !== false) {
    $data = explode(',', $cookie);
    $cookie = '';
    foreach ($data as $value) {
      $cookie .= current(explode(';', $value, 2)) . ';';
    }
    $cookie = rtrim($cookie, ';');
  }

  return $cookie;
}

/**
 * HTTP 1.1 Transfer-Encoding: chunked 过滤
 * @param (string) $data
 *  http 返回主体
 * @return (string) 去除 Transfer-Encoding 标识
 */
function dd_filter_unchunkHttp11($data) {
  $fp = 0;
  $outData = "";
  while ($fp < strlen($data)) {
    $rawnum = substr($data, $fp, strpos(substr($data, $fp), "\r\n") + 2);
    $num = hexdec(trim($rawnum));
    $fp += strlen($rawnum);
    $chunk = substr($data, $fp, $num);
    $outData .= $chunk;
    $fp += strlen($chunk);
  }
  return $outData;
}

// 文本错误信息
function dd_error_msg($msg) {
  global $user;
  $error = array(
    '错误信息：'. $msg,
    '发生时间：'. format_date(time()),
    '当前用户：'.(!empty($GLOBALS['user']->name) ? $GLOBALS['user']->name : '游客'),
    '用户主机：'. ip_address(),
    '当前页面：'. isset($_SERVER['REQUEST_URI']) ? $_SERVER['REQUEST_URI'] : '未知'
  );
  return implode("\n", $error);
}

/**
 * 写入错误日志，保存在配置目录
 * @param (string) $filename
 *  文件名。最终文件名将附加年月
 * @param (string) $msg
 *  错误信息
 */
function text_log($filename, $msg) {
  $filepath = DIDA_ROOT . '/sites/logs/' . $_SERVER['HTTP_HOST'] . '_' . $filename . '_' . date('Y_m_d') . '.php';
  $handle = fopen($filepath, 'ab');
  fwrite($handle, '<?php exit; ?>' . "\n" . $msg. "\n<!-- 分隔符 -->\n");
  fclose($handle);
}

/**
 * 以 <strong> 包裹字符串
 * @param (string) $text
 * @param (string) $sufpix
 * @return (string)
 */
function dd_get_strong($text, $sufpix = '：') {
  return '<strong>' . $text.$sufpix . '</strong>';
}

/**
 * 邮件发送，默认使用 mail() 发
 * @param (array) $mails
 *  数组，包含发件人等(键名区分大小写)：
 *    To：(array) 一组收件人的姓名和邮址
 *    例1：$mails = 'user@example.com'; // 仅一个邮址，若需提供姓名或一组：
 *    例2：$mails = array(array('user@c.com', 'name'), 'user1@example.com', )
 * @param (string) $subject
 *  邮件主题，换行符将被过滤
 * @param (string) $message
 *  邮件正文，每行不超过70
 * @param (string) $subject
 *  邮件主题，纯文本将在每70个字符后插入换行符，html 无此限制
 * @param (bool) $html
 *  发送为 html 内容
 * @param (array) $headers
 *  可提供更多邮件头信息，如抄送等
 * @param (array) $att
 *  更多配置，如可发送附件等。mail() 发送不支持
 * @return
 *  成功返回 true，失败返回 false，并发出一条警告
 */
//function dd_set_mail($to = array(), $subject, $message, $att = NULL, $filepath = NULL) {
function dd_set_mail($mails, $subject, $message, $headers = NULL, $html = 1, $att = NULL) {
  // @todo 待完善
  if (is_array($mails)) {
    foreach ($mails as $add) {
      if (is_array($add)) {
        // 邮址+姓名：array('user@example.com', 'name')
        if ($add[1]) {
          $tos[$add[0]] = $add[1].' <'.$add[0].'>';
        } else {
          $tos[$add[0]] = $add[0];
        }
      } else {
        $tos[$add] = $add;
      }
    }
    $to = implode(', ', $tos);
  } else {
    $to = $mails;
  }

  $data_header = array();

  if ($headers) {
    foreach ($headers as $key => $value) {
      $data_header[$key] = $key.':'.$value;
    }
    $header = implode("\r\n", $data_header);
  }

  if (!$html) {
    $message = wordwrap($message, 70, "\n", true);
  } else {
    $header .= 'MIME-Version: 1.0' . "\r\n";
    $header .= 'Content-type: text/html; charset=utf-8' . "\r\n";
  }
  
  // mb_language('utf-8');
  mb_language('uni');

  // 发件人
  if (empty($headers['From'])) {
    $header .= 'From: '.mb_encode_mimeheader($GLOBALS['conf']['site_global']['name'], 'utf-8', 'Q').' <'.var_get_key('site_global', 'send_mail', 'admin@'.$_SERVER['HTTP_HOST']).">\r\n";
  }
  
  if (mb_send_mail($to, $subject, $message, $header)) {
    return true;
  } else {
    dd_set_message(t('system', '邮件发送失败'), 'error');
    return false;
  }

  /*
   date_default_timezone_set('America/Toronto');
   require_once('./tools/class.phpmailer.php');
   $mail = new PHPMailer();
   if ($message) {
   $body = eregi_replace("[\]", '', $message);
   } else {
   if ($filepath && is_file($filepath)) {
   $body = file_get_contents($filepath);
   $body = eregi_replace("[\]", '', $body);
   }
   }
   $mail->IsSMTP();
   $mail->Host       = 'ssl://smtp.gmail.com';
   $mail->SMTPDebug  = 0;
   $mail->SMTPAuth   = true;
   $mail->Port       = 465;
   $mail->Username   = "yd2004@gmail.com";
   $mail->Password   = "......";
   $mail->SetFrom('yd2004@gmail.com', $GLOBALS['conf']['site_name']);

   $mail->Subject = $subject;

   $mail->AltBody = '你的邮箱不支持 HTML 格式。'; 

   $mail->MsgHTML($body);

   $name = $to[1] ? $to[1] : end(explode('@', $to[0]));
   $mail->AddAddress($to[0], $name);

   if ($att) {
   foreach ($att as $file) {
   $mail->AddAttachment($file);
   }
   }

   if (!$mail->Send()) {
   text_log('mail', '邮件发送错误: ' . $mail->ErrorInfo);
   } else {
   return true;
   }

   */
}

/**
 * 获取 IP 地理位置
 * @param string $ip
 *  ip 地址
 * @param bool $true
 *  在括号内显示地理信息，如：9.9.9.9.9(中国)。默认为：中国
 * @param bool $public
 *  公开整个 ip，若为 false，则以 * 替换 ip 结尾部分
 * @param (string) $function
 *  调用函数，默认使用 纯真 ip 库函数
 * @return string 地理信息，如：9.9.9.9.9(中国)，或：中国
 */
function dd_get_ip($ip, $true = NULL, $public = true, $function = 'qqwry_data')  {
  static $ips;

  if (!$ip) return false;

  if (!isset($ips[$ip])) {
    $return = '';
    if (preg_match("/^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/", $ip)) {
      $iparray = explode('.', $ip);
      if ($iparray[0] == 10 || $iparray[0] == 127 || ($iparray[0] == 192 && $iparray[1] == 168) || ($iparray[0] == 172 && ($iparray[1] >= 16 && $iparray[1] <= 31))) {
        $t = t('system', '本机地址'); 
      } else if ($iparray[0] > 255 || $iparray[1] > 255 || $iparray[2] > 255 || $iparray[3] > 255) {
        $t = t('system', 'IP不存在');
      } else if (function_exists($function)) {
        $t = $function($ip);
      } else {
        $t = t('system', '未知IP');
      }
    } else {
      $t = t('system', 'IP不合法');
    }
     
    if ($true) {
      if ($public) {
        $ips[$ip] = $ip . '(' . $t .')';
      } else {
        $ips[$ip] = $iparray[0] . '.' . $iparray[1] . '.' . $iparray[2] . '.*(' . $t .')';
      }
    } else {
      $ips[$ip] = $t;
    }
  }

  return $ips[$ip];
}

/**
 * 使用纯真 ip 库解析地理位置
 * @param (string) $ip
 *  ip 地址
 * @param (string) $file
 *  纯真 ip 库位置
 * @return (string)
 */
function qqwry_data($ip, $file = './tools/QQWry.Dat') {
  if (!is_file($file)) return false;

  if (!$fd = fopen($file, 'rb')) {
    return false;
  }

  $ip = explode('.', $ip);
  $ipNum = $ip[0] * 16777216 + $ip[1] * 65536 + $ip[2] * 256 + $ip[3];

  if (!($DataBegin = fread($fd, 4)) || !($DataEnd = fread($fd, 4)) ) return;
  @$ipbegin = implode('', unpack('L', $DataBegin));
  if ($ipbegin < 0) $ipbegin += pow(2, 32);
  @$ipend = implode('', unpack('L', $DataEnd));
  if ($ipend < 0) $ipend += pow(2, 32);
  $ipAllNum = ($ipend - $ipbegin) / 7 + 1;

  $BeginNum = $ip2num = $ip1num = 0;
  $ipAddr1 = $ipAddr2 = '';
  $EndNum = $ipAllNum;

  while ($ip1num > $ipNum || $ip2num < $ipNum) {
    $Middle= intval(($EndNum + $BeginNum) / 2);

    fseek($fd, $ipbegin + 7 * $Middle);
    $ipData1 = fread($fd, 4);
    if (strlen($ipData1) < 4) {
      fclose($fd);
      return false;
    }
    $ip1num = implode('', unpack('L', $ipData1));
    if ($ip1num < 0) $ip1num += pow(2, 32);

    if ($ip1num > $ipNum) {
      $EndNum = $Middle;
      continue;
    }

    $DataSeek = fread($fd, 3);
    if (strlen($DataSeek) < 3) {
      fclose($fd);
      return false;
    }
    $DataSeek = implode('', unpack('L', $DataSeek.chr(0)));
    fseek($fd, $DataSeek);
    $ipData2 = fread($fd, 4);
    if (strlen($ipData2) < 4) {
      fclose($fd);
      return false;
    }
    $ip2num = implode('', unpack('L', $ipData2));
    if ($ip2num < 0) $ip2num += pow(2, 32);

    if ($ip2num < $ipNum) {
      if ($Middle == $BeginNum) {
        fclose($fd);
        return t('system', '未知');
      }
      $BeginNum = $Middle;
    }
  }

  $ipFlag = fread($fd, 1);
  if ($ipFlag == chr(1)) {
    $ipSeek = fread($fd, 3);
    if (strlen($ipSeek) < 3) {
      fclose($fd);
      return false;
    }
    $ipSeek = implode('', unpack('L', $ipSeek.chr(0)));
    fseek($fd, $ipSeek);
    $ipFlag = fread($fd, 1);
  }

  if ($ipFlag == chr(2)) {
    $AddrSeek = fread($fd, 3);
    if (strlen($AddrSeek) < 3) {
      fclose($fd);
      return false;
    }
    $ipFlag = fread($fd, 1);
    if ($ipFlag == chr(2)) {
      $AddrSeek2 = fread($fd, 3);
      if (strlen($AddrSeek2) < 3) {
        fclose($fd);
        return false;
      }
      $AddrSeek2 = implode('', unpack('L', $AddrSeek2.chr(0)));
      fseek($fd, $AddrSeek2);
    } else {
      fseek($fd, -1, SEEK_CUR);
    }

    while (($char = fread($fd, 1)) != chr(0))
    $ipAddr2 .= $char;

    $AddrSeek = implode('', unpack('L', $AddrSeek.chr(0)));
    fseek($fd, $AddrSeek);

    while (($char = fread($fd, 1)) != chr(0))
    $ipAddr1 .= $char;
  } else {
    fseek($fd, -1, SEEK_CUR);
    while (($char = fread($fd, 1)) != chr(0))
    $ipAddr1 .= $char;

    $ipFlag = fread($fd, 1);
    if ($ipFlag == chr(2)) {
      $AddrSeek2 = fread($fd, 3);
      if (strlen($AddrSeek2) < 3) {
        fclose($fd);
        return false;
      }
      $AddrSeek2 = implode('', unpack('L', $AddrSeek2.chr(0)));
      fseek($fd, $AddrSeek2);
    } else {
      fseek($fd, -1, SEEK_CUR);
    }
    while (($char = fread($fd, 1)) != chr(0))
    $ipAddr2 .= $char;
  }
  
  fclose($fd);

  if (preg_match('/http/i', $ipAddr2)) {
    $ipAddr2 = '';
  }
  
  $ipaddr = "$ipAddr1 $ipAddr2";
  $ipaddr = preg_replace('/CZ88\.NET/is', '', $ipaddr);
  $ipaddr = preg_replace('/^\s*/is', '', $ipaddr);
  $ipaddr = preg_replace('/\s*$/is', '', $ipaddr);
  if (preg_match('/http/i', $ipaddr) || $ipaddr == '') {
    $ipaddr = t('system', '未知');
  }
  return iconv("gb2312", "UTF-8", $ipaddr);
}

/**
 * 判断是否为蜘蛛
 */
function dd_is_spider() {
  return strpos($_SERVER['HTTP_USER_AGENT'], 'spider') !== false 
  || strpos($_SERVER['HTTP_USER_AGENT'], 'bot') !== false 
  || strpos($_SERVER['HTTP_USER_AGENT'], 'Yahoo!') !== false;
}

/**
 * 载入 jquery ui，
 * @param string $theme
 *  载入的主题
 */
// public dd_jqui($theme = NULL) {{{ 
/**
 * dd_jqui
 *  载入 jquery ui
 * 
 * @param string $theme 
 *  指定主题，若未指定，使用系统设置
 * @access public
 * @return void
 */
function dd_jqui($theme = NULL) {
  if (empty($theme)) {
    $theme = $GLOBALS['conf']['base_ui_theme'];
  }
  dd_add_js('misc/jquery.bgiframe.js');
  dd_add_js('misc/ui/jquery-ui.js');
  dd_add_js('misc/ui/jquery-ui-i18n.js');
  dd_add_css('misc/ui/themes/' . $theme . '/jquery-ui.min.css');
}
// }}}

// public dd_load_jquery_plugin($name) {{{ 
/**
 * dd_load_jquery_plugin
 *  加载 jquery 插件
 * 
 * @param string $name 
 *  插件名称
 * @access public
 * @return void
 */
function dd_load_jquery_plugin($name) {
  static $data;

  if (empty($data[$name])) {
    dd_add_js('misc/jquery/' . $name . '.js');
    $data[$name] = 1;
  }
}
// }}}

/**
 * 载入 jqedit 插
 * @param (array) $v
 *  自定义参数
 */
function dd_jqedit($v = array()) {
  $args = array(
    'indicator' => '<img src="' . f('misc/images/loading.gif') . '">',
    'submit' => t('system', '确定'),
    'cancel' => t('system', '取消'),
    'tooltip' => t('system', '点此编辑'),
  );

  $v['opt'] = array_merge($args, $v['opt']);

  if ($v['limit']) $v['opt']['charcounter']['characters'] = $v['limit'];

  dd_add_js(array('edit' => array($v)), 'setting');

  dd_add_js('misc/jquery.jeditable.js');

  switch ($v['opt']['type']) {
    case 'charcounter':
      dd_add_js('misc/jquery.jeditable.charcounter.js');
      dd_add_js('misc/jquery.charcounter.js');
      break;
    case 'autogrow':
      dd_add_js('misc/jquery.jeditable.autogrow.js');
      dd_add_js('misc/jquery.autogrow.js');
      break;
    case 'masked':
      dd_add_js('misc/jquery.jeditable.masked.js');
      dd_add_js('misc/jquery.maskedinput.js');
      break;
    case 'datepicker':
      dd_add_js('misc/jquery.jeditable.datepicker.js');
      dd_add_js('misc/jquery.datepicker.js');
      dd_add_js('misc/data.js');
      break;
    case 'timepicker':
      dd_add_js('misc/jquery.jeditable.timepicker.js');
      dd_add_js('misc/jquery.timepicker.js');
      break;
    case 'ajaxupload':
      dd_add_js('misc/jquery.jeditable.ajaxupload.js');
      dd_add_js('misc/jquery.ajaxfileupload.js');
      break;
    case 'time':
      dd_add_js('misc/jquery.jeditable.time.js');
      dd_add_js('misc/jquery.dimensions.js');
  }
}

/**
 * jquery dialog 弹出层设置
 * @param (int) $height
 *  高度
 * @param (int) $width
 *  宽度
 * @param (bool) $iframe
 *  以 iframe 模式加载
 * @param (string) $inlineId
 *  显示页面指定 id 的内
 * @param (bool) $reload
 *  关闭后是否刷新当前页
 * @return (array) 可用于 url() 的数
 *
 */
function dd_dialog_att($height = 400, $width = 600, $iframe = true, $inlineId = NULL, $reload = NULL) {
  $str = "width=$width&height=$height";
  if ($iframe) {
    $str .= '&iframe=true&modal=true';
  }
  if ($inlineId) {
    $str .= '&inlineId='.$inlineId;
  }
  if ($reload) {
    $str .= '&reload=true';
  }
  dd_jqui();
  return array('query' => $str, 'attributes' => array('class' => 'dialog', 'target' => '_blank'));
}

/**
 * 写入提示信息
 * @param (string) $message
 *  信息内容
 * @param (string) $type
 *  信息类型，可自定义，常用：notice、error、success、warning
 * @param (bool) $repeat
 *  是否替换旧的相同类型的提示信息
 */
function dd_set_message($message = NULL, $type = 'notice', $repeat = true) {
  if ($message) {
    if (is_array($message)) $message = dd_array_to_string($message);
    
    if ($type == 'sql_bug' && !$GLOBALS['conf']['debug']) {
      text_log('sql_bug', $message);
      if (!$GLOBALS['conf']['debug']) {
        return false;
      }
      $type = 'debug';
    }
    if (!isset($_SESSION['messages'])) {
      $_SESSION['messages'] = array();
    }
    if (!isset($_SESSION['messages'][$type])) {
      $_SESSION['messages'][$type] = array();
    }
    if ($repeat || !in_array($message, $_SESSION['messages'][$type])) {
      $_SESSION['messages'][$type][] = $message;
    }
  }
}

/**
 * 获取提示信息
 * @return (string)
 */
function dd_get_message() {
  if (!empty($_SESSION['messages']) && function_exists('theme_get_element')) {
    $output = theme_get_element('dd_element_message', $_SESSION['messages']);
    unset($_SESSION['messages']);
    return $output;
  }
}

// public dd_element_message($messages) {{{ 
/**
 * dd_element_message
 * 解析提示信息
 * 
 * @param array $messages 
 * @access public
 * @return string
 */
function dd_element_message($messages) {
  $output = '';
  foreach ($messages as $type => $data) {
    $output .= "<div class=\"messages $type\">\n";
    if (count($data) > 1) {
      $output .= " <ul>\n";
      foreach ($data as $message) {
        $output .= '  <li>'. $message ."</li>\n";
      }
      $output .= " </ul>\n";
    } else {
      $output .= $data[0];
    }
    $output .= "</div>\n";
  }
  return $output;
}
// }}}

/**
 * 将数组格式化输出，用于调试
 * @param (array) $array
 * @return (string)
 */
function dd_array_to_string($array) {
  ob_start();
  var_dump($array);
  $output = ob_get_contents();
  ob_end_clean();
  return $output;
}

/**
 * 写入帮助信息
 * @param (string) $help
 * @return (string)
 */
function dd_set_help($help = NULL) {
  static $data;
  if ($help) {
    $data[] = $help;
  }
  return $data;
}

/**
 * 获取帮助信息，在当前页显示
 * @return (string)
 */
function dd_get_help() {
  if ($help = dd_set_help()) {
    return theme('help', $help);
  }
}

/**
 * 写入页脚信息
 * @param (string) $item
 */
function dd_set_footer($item = NULL) {
  static $items;
  if ($item) {
    $items[] = $item;
  } else {
    return $items;
  }
}

/**
 * 获取页脚信息
 * @return (string)
 */
function dd_get_footer() {
  if ($items = dd_set_footer()) {
    return theme('item_list', $items, NULL, 'ul', array('id' => 'module_invoke_footer'));
  }
}

// public dd_set_tabs($tab = NULL, $clear = false) {{{ 
/**
 * dd_set_tabs
 *  设置 tabs
 * 
 * @param string|array $tab 
 *  一个或多个 tab
 * @param bool $clear 
 *  清除 $tabs
 * @access public
 * @return void
 */
function dd_set_tabs($tab = NULL, $clear = false) {
  static $tabs;
  if (!empty($clear)) {
    $tabs = NULL;
  }
  if ($tab) {
    if (is_array($tab)) {
      foreach ($tab as $t) {
        $tabs[] = $t;
      }
    } else {
      $tabs[] = $tab;
    }
  } else {
    return $tabs;
  }
}
// }}}

// public dd_element_sub_tabs($tabs) {{{ 
/**
 * dd_element_sub_tabs
 * 格式化 sub tabs
 * 
 * @param array $tabs 
 * @access public
 * @return string
 */
function dd_element_sub_tabs($tabs) {
  return theme('item_list', $tabs, NULL, 'ul', array('class' => 'sub_tabs'));
}
// }}}

/**
 * 获取 sub tabs 导航
 * @return (string)
 */
function dd_get_sub_tabs() {
  if ($tabs = dd_set_sub_tabs()) {
    return theme_get_element('dd_element_sub_tabs', $tabs);
  }
}

// public dd_set_sub_tabs($tab = NULL, $clear = false) {{{ 
/**
 * dd_set_sub_tabs
 *  设置 sub tabs 导航
 * 
 * @param string|array $tab 
 *  一个或多个 tab
 * @param bool $clear 
 *  清除 $tabs
 * @access public
 * @return void
 */
function dd_set_sub_tabs($tab = NULL, $clear = false) {
  static $tabs;
  if (!empty($clear)) {
    $tabs = NULL;
  }
  if ($tab) {
    if (is_array($tab)) {
      foreach ($tab as $t) {
        $tabs[] = $t;
      }
    } else {
      $tabs[] = $tab;
    }
  } else {
    return $tabs;
  }
}
// }}}

// public dd_get_tabs() {{{ 
/**
 * dd_get_tabs
 * 获取 tabs
 * 
 * @access public
 * @return void
 */
function dd_get_tabs() {
  if ($tabs = dd_set_tabs()) {
    return theme_get_element('dd_element_tabs', $tabs);
  }
}
// }}}

// public dd_element_tabs($tabs) {{{ 
/**
 * dd_element_tabs
 * 格式化 tabs
 * 
 * @param array $tabs 
 * @access public
 * @return string
 */
function dd_element_tabs($tabs) {
  return theme('item_list', $tabs, NULL, 'ul', array('class' => 'tabs'));
}
// }}}

/**
 * 写入页面标题
 * @param (string) $title
 */
function dd_set_title($title = NULL) {
  static $stored_title;
  if (isset($title)) {
    $stored_title = $title;
  } else {
    return $stored_title;
  }
}

/**
 * 获取页面标题
 * @return (string)
 */
function dd_get_title() {
  if ($title = dd_set_title()) {
    $head_title = implode(' - ', array_filter($title)) .' - ';
  }
  $head_title .= $GLOBALS['conf']['site_global']['name'];
  if ($GLOBALS['conf']['site_global']['slogan']) {
    $head_title .= ' - ' . $GLOBALS['conf']['site_global']['slogan'];
  }
  return $head_title;
}

/**
 * 写入导航
 * @param (string or array) $breadcrumb
 *  导航链接
 * @param (bool) $front
 *  是否在前面显示 首页
 */
function dd_set_breadcrumb($breadcrumb = NULL, $front = true) {
  static $value;
  if (is_array($breadcrumb)) {
    if ($front) array_unshift($breadcrumb, l(t('system', '首页'), NULL));
    $value = $breadcrumb;
  } else {
    return $value;
  }
}

/**
 * 获取导航
 * @return (string)
 */
function dd_get_breadcrumb() {
  if ($breadcrumb = dd_set_breadcrumb()) {
    return theme('breadcrumb', array_filter($breadcrumb));
  }
}

/**
 * 获取下拉导航，触发 hook_site_menu()
 * @param array $menus
 *  导航菜单，若无，则执行 hook_site_menu
 * @param bool $first
 *  第一次调用，添加 js 代码，以实现下拉效果
 * @return (string)
 */
function dd_get_menu($menus = NULL, $first = true) {
  if (!isset($menus)) {
    $menus = array();
    module_alter_all('site_menu', $menus);
    $class = 'site_menu';
  } else {
    $class = 'site_menu_children';
  }

  if ($menus) {
    static $i;

    if (!empty($first)) {
      $custom_data = var_get('site_menu_system', array());
    }

    foreach ($menus as $key => $lists) {
      if (!empty($first)) {
        // 已禁用的导航
        if (!empty($custom_data['disabled']) && isset($custom_data['disabled'][$key])) continue;
        
        // 可通过 hook_site_menu_alter() 覆写 menu
        module_alter_all('site_menu_alter', $key, $lists);

        // 自定义权重
        if (!empty($custom_data['weight']) && !empty($custom_data['weight'][$key])) {
          if (is_array($lists)) {
            $lists['#weight'] = $custom_data['weight'][$key];
          } else {
            $lists = array(
              '#data' => $lists,
              '#weight' => $custom_data['weight'][$key],
            );
          }
        }
      }

      if (is_array($lists)) {
        if ($lists['#data']) {
          $data = '';
          $data .= $lists['#data'];
          if ($lists['#childrens']) {
            $data .= dd_get_menu($lists['#childrens'], 0);
          }
          $items[] = array(
            '#data' => array('data' => $data, 'class' => 'site_menu_list site_menu_list_' . $key),
            '#weight' => ($lists['#weight'] ? $lists['#weight'] : $weight)
          );
        } else if (!$lists['#childrens']) {
          foreach ($lists as $k => $child) {
            if (is_array($child)) {
              if ($child['#data']) {
                $data = '';
                $data .= $child['#data'];
                if ($child['#childrens']) {
                  $data .= dd_get_menu($child['#childrens'], 0);
                }
                $items[] = array(
                  '#data' => array('data' => $data, 'class' => 'site_menu_list site_menu_list_' . $k),
                  '#weight' => ($child['#weight'] ? $child['#weight'] : $weight)
                );
              }
            } else {
              $items[] = array(
                '#data' => array('data' => $child, 'class' => 'site_menu_list site_menu_list_' . $k),
                '#weight' => $i
              );
            }
            ++$i;
          }
        }
      } else {
        $items[] = array(
          '#data' => array('data' => $lists, 'class' => 'site_menu_list site_menu_list_' . $key),
          '#weight' => $i,
        );
      }
      ++$i;
    }

    if ($items) {
      uasort($items, 'dd_form_cmp');
      $j = 1;
      foreach ($items as $d) {
        if (!empty($first)) {
          $d['#data']['class'] .= ' site_menu_list_index_' . $j;
        } else {
          $d['#data']['class'] .= ' site_menu_list_index_sub_' . $j;
        }
        $item[] = $d['#data'];
        $j++;
      }
      $output = theme('item_list', $item, NULL, 'ul', array('class' => $class), 0);
    }
    return $output;
  }
}

/**
 * 写入 header
 * @param (string) $header
 */
function dd_set_header($header = NULL) {
  static $stored_headers = array();
  if (strlen($header)) {
    header($header);
    $stored_headers[] = $header;
  }
  return implode("\n", $stored_headers);
}

/**
 * 写入 head
 * @param (string) $data
 * @return (string)
 */
function dd_set_html_head($data = NULL) {
  static $stored_head = '';
  if (!is_null($data)) {
    $stored_head .= $data ."\n";
  }
  return $stored_head;
}

/**
 * 获取 head
 * @return (string)
 */
function dd_get_html_head() {
  return dd_set_html_head();
}

/**
 * 向页面添加 css 或文件
 * @param (string) $path
 *  css 或 css 文件路径
 * @param (string) $type
 *  添加类型，该类型决定加载顺序：core -> module -> theme
 *  特殊类型 inline，直接输出一段 css
 * @param (string) $media
 *  media 类型
 * @param (bool) $cached
 *  是否缓存
 * @return (array)
 */
function dd_add_css($data = NULL, $type = 'module', $media = 'all', $cached = true) {
  static $css = array(), $weight = array();

  if (isset($data)) {

    if (empty($css)) {
      $css[$media] = array('__inline' => '', 'files' => array());
      $weight = array('core' => 0, 'module' => 100, 'theme' => 1000);
    }

    if ($type != 'inline') {
      $css['files'][$data] = array(
        'cached' => $cached, 
        'type' => $type, 
        'media' => $media, 
        'weight' => $weight[$type]++
      );
    } else {
      $css['__inline'] .= $data;
    }
  }

  return $css;
}

/**
 * 获取所有通过 dd_add_css 添加的 css
 * @param (string) $css
 *  css 文件路径
 * @return (string) css html 标记
 */
function dd_get_css($css = NULL) {
  global $base_path, $conf, $base_theme_name, $base_theme_info;

  if (!isset($css)) {
    $css = dd_add_css();
  }

  $output = '';

  /**
   * 在主题 template.php 中，可实现 hook_alter_css 来覆写加载的 css 文件，如：
   * function default_alter_css(&$css) {
   *   unset($css['modules/system/system.css']);
   * }
   *
   * 也可以在 theme info 中移除指定 css、js 文件，如：
   * 'styles_remove' => array(
   *   'modules/system/admin.css' => true,
   * ),
   * 
   */
  module_alter($base_theme_name, 'alter_css', $css);

  if (!empty($css['files'])) {
    $filename = '';
    $css_path = array();
    uasort($css['files'], 'dd_sort_cmp');

    foreach ($css['files'] as $path => $info) {
      if (!empty($base_theme_info['styles_remove'][$path])) continue;
      // 只缓存站内 css 文件
      if (strpos($path, 'http://') === false && strpos($path, 'https://') === false) {
        if ($GLOBALS['conf']['cache_css_and_js'] && $info['cached']) {
          $css_path[$info['type']][] = $path;
          $filename .= $path;
          continue;
        }
        $path = $base_path . $path;
      }

      if (strpos($path, '?') === false) {
        $path .= '?'.$conf['cache_css_and_js_timestamp'];
      } else {
        $path .= '&'.$conf['cache_css_and_js_timestamp'];
      }

      $output .= '<link type="text/css" rel="stylesheet" media="' 
        . $info['media'] .'" href="'. $path.'" />'."\n";
    }

    if ($css_path && $filename) {
      $filepath = file_directory_path() . '/cache/css_js/' 
        . md5($base_theme_name . $filename) . '.css';

      if (!is_file($filepath)) {
        foreach (array('core', 'module', 'theme') as $key) {
          if ($css_path[$key]) {
            foreach ($css_path[$key] as $path) {
              $data = file_get_contents($path);
              $base = $base_path . dirname($path) . '/';
              _dd_build_css_path(NULL, $base);
              $contents .= preg_replace_callback('/url\([\'"]?(?![a-z]+:|\/+)([^\'")]+)[\'"]?\)/i', '_dd_build_css_path', $data);
            }
          }
        }
        _file_save_data($contents, $filepath);
      }

      $output .= '<link type="text/css" rel="stylesheet" media="all" href="'. $base_path . $filepath . '?' . $conf['cache_css_and_js_timestamp'] . '" />'."\n";
    }
  }


  if (!empty($css['__inline'])) {
    $output .= "<style>\n" . $css['__inline'] . "\n</style>\n";
  }

  return $output;
}

/**
 * css 文件合并缓存，背景图片路径替换
 * @param (array) $matches
 *  正则匹配数据
 * @param (string) $base
 *  css 文件路径
 */
function _dd_build_css_path($matches, $base = NULL) {
  static $_base;
  if (isset($base)) {
    $_base = $base;
  }
  $path = $_base . $matches[1];

  /*
   $last = '';
   while ($path != $last) {
   $last = $path;
   $path = preg_replace('`(^|/)(?!../)([^/]+)/../`', '$1', $path);
   }
   */
  return 'url('. $path .')';
}

/**
 * 将 js 输出在页面，并中止代码
 * @param (string) $js
 *  js 代码
 */
function dd_js_print($js) {
  print "$(function() {\n". $js . "\n});";
  exit;
}

/**
 * 为 js 代码添加 script 标签
 * @param (string) $js
 *  js 代码
 * @return (string)
 */
function _dd_js_print($js) {
  return '<script type="text/javascript">' . $js . '</script>';
}

/**
 * js 页面跳转
 * @param (int) $limit
 *  页码，或其它标识
 */
function dd_js_goto($limit) {
  if ($q = dd_query_string_encode($_REQUEST, array_merge(array('q', 'js_limit'), array_keys($_COOKIE)))) {
    $q = '&' . $q;
  }

  if ($_GET['js_limit']) $limit += $_GET['js_limit'];

  $path = url($_GET['q'], array('query' => 'js_limit='.$limit.$q));
  echo _dd_js_print('location.href = "'.$path.'";');
  exit;
}

/**
 * 以指定字符分割数组，并以  span 包裹
 * @param (array) $v
 *  待操作数组
 * @param (string) $str
 *  分隔符
 */
function dd_imp($v = array(), $str = ' | ') {
  if ($v) return '<span class="dd_imp">'.implode($str, $v).'</span>';
}

/**
 * 获取排序链接，可用于通过 GET 参数排序
 * 函数将使用 ext_sort 和 ext_order 两个参数，以与 theme_table 的排序区别
 * @param (array) $sort
 *  排序参数，例如： 
 *    array(
 *      'timestamp' => array('data' => '时间'),
 *      'visit' => array('data' => '访问', 'sort' => 'desc'), // 若有 sort 参数，则表示此字段为默认值
 *    );
 * @param (string) $path
 *  链接路径，如：user/login，默认为 $_GET['q']
 * @param (array) $exclude
 *  默认将把当前 $_GET 值保留，若不需要保留某些值，在此处指明，如：arraqy('page', 'order')
 * @return (string) 返回 html 链接
 */
function dd_get_sort_link($sort, $path = NULL, $exclude = NULL) {
  
  if (!isset($path)) {
    $path = $_GET['q'];
  }
  
  $query = $_GET;
  
  if (isset($exclude) && is_array($exclude)) {
    foreach ($exclude as $key) {
      if (isset($query[$key])) {
        unset($query[$key]);
      }
    }
  }
  
  unset($query['page'], $query['q']);
  
  $tags = array('asc' => 'desc', 'desc' => 'asc');
  
  foreach ($sort as $s => $v) {
    $att = array();
    $att['query'] = $query;
    $att['query']['ext_sort'] = $s;
    $att['attributes']['class'] = 'ext_sort_order';
    if (is_array($v)) {
      if (empty($v['data'])) {
        continue;
      }
      
      $title = $v['data'];
      
      if (empty($_GET['ext_sort'])) {
        if (isset($v['sort']) && in_array($v['sort'], array('asc', 'desc'))) {
          $att['attributes']['class'] .= ' sort_active_' . $v['sort']; 
          $att['query']['ext_order'] = $tags[$v['sort']];
        }
      } else if (isset($v['sort']) && $_GET['ext_sort'] == $s) {
        $att['attributes']['class'] .= ' sort_active_' . $_GET['ext_order'];
        $att['query']['ext_order'] = $tags[$_GET['ext_order']];
      } else {
        $att['query']['ext_order'] = 'desc';
      }
      
    } else {
      
      if (!empty($_GET['ext_sort']) && $s == $_GET['ext_sort'] 
      && !empty($_GET['ext_order']) && in_array($_GET['ext_order'], array('asc', 'desc'))) {
        $att['attributes']['class'] .= ' sort_active_' . $_GET['ext_order'];
        $att['query']['ext_order'] = $tags[$_GET['ext_order']];
      } else {
        $att['query']['ext_order'] = 'desc';
      }
      
      $title = $v;
    }
    
    $output .= l($title, $path, $att);
    
  }
  
  return $output;
}

/**
 * 向当前页面添加 js
 * @param (string) $data
 *  js 文件路径
 * @param (string) $type
 *  类型，决定加载顺序：core -> module -> theme
 * @param (bool) $cache
 *  是否合并缓存
 * @param (string) $scope
 *  加载位置：header、footer
 * @param (bool) $defer
 *  defer 模式
 */
function dd_add_js($data = NULL, $type = 'module', $cached = true, $scope = 'header', $defer = false) {
  static $js = array(), $weight = array();

  if (!empty($data)) {

    if (empty($js)) {
      $js = array(
        'header' => array('__inline' => '', 'setting' => array()),
        'footer' => array('__inline' => '', 'setting' => array()),
      );
      // 设置权重，保证加载顺序为 croe -> module -> theme
      $weight = array('croe' => 0, 'module' => 100, 'theme' => 1000);
    }

    switch ($type) {
      case 'inline':
        $data =  '$(function() {' . $data . '})';
      case 'inline_nojq':
        $js[$scope]['__inline'] .=  $data . ";\n";
      break;
      case 'setting':
        $js[$scope]['setting'][] = $data;
      break;
      default:
        $js[$scope]['files'][$data] = array(
          'defer' => $defer,
          'cached' => $cached,
          'type' => $type,
          'weight' => $weight[$type]++
        );
    }
  }

  return !empty($scope) ? $js[$scope] : $js;
}

/**
 * 获取当前页面加载的 js
 * @param (string) $scope
 *  位置，header 或 footer
 * @param (array) $js
 *  js 文件
 * @return (string)
 */
function dd_get_js($scope = 'header', $js = NULL) {
  global $conf, $base_path, $base_theme_name, $language_code, $base_theme_info;
  static $init;
  
  if (empty($init)) {
    dd_add_js(array(
      'cache_css_and_js_timestamp' => $conf['cache_css_and_js_timestamp'],
      'base_theme' => $conf['base_theme'],
      'base_ui_theme' => $conf['base_ui_theme'],
      'base_path' => $base_path,
      'language_code' => $language_code, 
      'clean_url' => $conf['clean_url'] ? true : false,
      'user_is_login' => !empty($GLOBALS['user']->uid)
    ), 'setting');
  }

  if (!isset($js)) {
    $js = dd_add_js(NULL, NULL, NULL, $scope);
  }

  $init = 1;

  /**
   * 在主题 template.php 中，可实现 hook_alter_js 来覆写加载的 js 文件，如：
   * function default_alter_js(&$css) {
   *   unset($css['modules/system/system.js']);
   * }
   */
  module_alter($base_theme_name, 'alter_js', $js, $scope);

  $js_path = array();
  $output = $inline = '';
  
  if (!empty($js['setting'])) {
    /**
     * 将 js 中的 Dida 对象，在这儿初始化，输出在页面顶端。
     * @TODO 为避免和其它 js 库内部变量造成冲突，将之前的全局配置变量 settings，修改为 Dida.settings
     *       为兼容之前的代码，暂不取消 settings 变量，做为过渡
     *       js 全局变量 settings 已取消
     */
    $output = "<script type=\"text/javascript\">\n<!--//--><![CDATA[//><!--\n";
    $output .= 'var Dida = {cache: {}, module: {';
    foreach ($conf['modules'] as $key => $module) {
      $output .= $key . ': {},';
    }
    $output = rtrim($output, ',') . '},';
    $output .= 'settings: ' . dd_to_js(call_user_func_array('array_merge_recursive', $js['setting']));
    $output .= "}\n//--><!]]>\n</script>\n";
  }

  if ($js['__inline']) {
    $inline = "<script type=\"text/javascript\">\n<!--//--><![CDATA[//><!--\n" 
      . $js['__inline'] . "\n//--><!]]>\n</script>";
  }

  if (empty($js['files'])) {
    return $output . $inline;
  }

  $filename = '';

  // 按 weight 排序
  uasort($js['files'], 'dd_sort_cmp');

  foreach ($js['files'] as $path => $info) {
    if (!empty($base_theme_info['scripts_remove'][$path])) continue;
    if ($conf['cache_css_and_js'] && $info['cached']) {
      $js_path[] = $path;
      $filename .= $path;
      continue;
    }

    if (strpos($path, 'http://') === false && strpos($path, 'https://') === false && substr($path, 0, 1) != '/') {
      $path = $base_path . $path;
    }

    if (strpos($path, '?') === false) {
      $path .= '?'.$conf['cache_css_and_js_timestamp'];
    } else {
      $path .= '&'.$conf['cache_css_and_js_timestamp'];
    }

    $output .= '<script type="text/javascript"'. ($info['defer'] ? ' defer="defer"' : '') 
      .' src="'. $path."\"></script>\n";

  }

  if ($js_path && $filename) {

    $filepath = file_mkdir(file_directory_path() . '/cache/css_js') . '/' 
      . md5($base_theme_name . $filename) . '.js';

    if (!is_file($filepath)) {
      foreach ($js_path as $path) {
        $contents .= file_get_contents($path).';';
      }
      _file_save_data($contents, $filepath);
    }

    $output .= '<script type="text/javascript" src="'. $base_path .$filepath .'?' . $conf['cache_css_and_js_timestamp'] . '"></script>'."\n";
  }

  return $output . $inline;
}

/**
 * 任意类型转换为 js
 * @param (*) $var
 * @return (string)
 */
function dd_to_js($var) {
  switch (gettype($var)) {
    case 'boolean':
      return $var ? 'true' : 'false';
    case 'integer':
    case 'double':
      return $var;
    case 'resource':
    case 'string':
      if (strpos($var, ':::code:::') === false) {
        return '"'. str_replace(array("\r", "\n", "<", ">", "&"),
        array('\r', '\n', '\x3c', '\x3e', '\x26'),
        addslashes($var)) .'"';
      } else {
        return str_replace(array(':::code:::'), array(''), $var);
      }
    case 'array':
      if (empty ($var) || array_keys($var) === range(0, sizeof($var) - 1)) {
        $output = array();
        foreach ($var as $v) {
          $output[] = dd_to_js($v);
        }
        return '[ '. implode(', ', $output) .' ]';
      }
    case 'object':
      $output = array();
      foreach ($var as $k => $v) {
        $output[] = dd_to_js(strval($k)) .': '. dd_to_js($v);
      }
      return '{ '. implode(', ', $output) .' }';
    default:
      return 'null';
  }
}

/**
 * 将字符串转换为数组，以换行符分割
 * @param (string) $string
 *  待处理的字符串
 * @param (bool) $repeat
 *  若为 true，则去除数组中空白和重复值
 */
function dd_line_to_array($string, $repeat = NULL) {
  if (!$string) return false;
  if (strpos($string, "\r\n") !== false) {
    $string = str_replace("\r\n", "\n", $string);
  }
  if (strpos($string, "\n") !== false) {
    $string = trim($string);
    if ($arr = explode("\n", $string)) {
      if ($repeat) {
        $arr = array_filter($arr);
        $arr = array_unique($arr);
      }
      return $arr;
    }
  } else {
    return array(trim($string));
  }
  return false;
}

/**
 * 将数组转换为 GET 字符串
 * @param (array) $query
 *  待转换的数组，如：array('op' => 'update', 'id' => 100)
 * @param (array) $exclude
 *  不转换的单元
 * @param (string) $parent
 *  父级名称，如：list，则上面的数组转换为：list[op]=update&list[id]=100
 * @return (string) 如：op=update&id=100
 */
function dd_query_string_encode($query = array(), $exclude = array(), $parent = '') {
  $params = array();
  foreach ($query as $key => $value) {
    $key = dd_urlencode($key);
    if ($parent) {
      $key = $parent .'['. $key .']';
    }
    if ($exclude && in_array($key, $exclude)) {
      continue;
    }
    
    if (is_object($value)) $value = (array) $value;

    if (is_array($value)) {
      $params[] = dd_query_string_encode($value, $exclude, $key);
    } else {
      $params[] = $key .'='. dd_urlencode($value);
    }
  }
  return implode('&', $params);
}

/**
 * 字符串编码
 * @param (string) $text
 * @param (array) $exclude
 */
function decode_entities($text, $exclude = array()) {
  static $table;
  if (!isset($table)) {
    $table = array_flip(get_html_translation_table(HTML_ENTITIES));
    $table = array_map('utf8_encode', $table);
    $table['&apos;'] = "'";
  }
  $newtable = array_diff($table, $exclude);
  return preg_replace('/&(#x?)?([A-Za-z0-9]+);/e', '_decode_entities("$1", "$2", "$0", $newtable, $exclude)', $text);
}

/**
 * 处理字符串
 */
function _decode_entities($prefix, $codepoint, $original, &$table, &$exclude) {
  if (!$prefix) {
    if (isset($table[$original])) {
      return $table[$original];
    } else {
      return $original;
    }
  }
  if ($prefix == '#x') {
    $codepoint = base_convert($codepoint, 16, 10);
  } else {
    $codepoint = preg_replace('/^0+/', '', $codepoint);
  }
  if ($codepoint < 0x80) {
    $str = chr($codepoint);
  } else if ($codepoint < 0x800) {
    $str = chr(0xC0 | ($codepoint >> 6))
    . chr(0x80 | ($codepoint & 0x3F));
  } else if ($codepoint < 0x10000) {
    $str = chr(0xE0 | ( $codepoint >> 12))
    . chr(0x80 | (($codepoint >> 6) & 0x3F))
    . chr(0x80 | ( $codepoint       & 0x3F));
  } else if ($codepoint < 0x200000) {
    $str = chr(0xF0 | ( $codepoint >> 18))
    . chr(0x80 | (($codepoint >> 12) & 0x3F))
    . chr(0x80 | (($codepoint >> 6)  & 0x3F))
    . chr(0x80 | ( $codepoint        & 0x3F));
  }
  if (in_array($str, $exclude)) {
    return $original;
  }
  else {
    return $str;
  }
}

/**
 * 字符串检查，去除 html 标签
 * @param (string) $text
 *  待检查的字符串
 * @return (string)
 */
function check_plain($text) {
  static $php525;

  if (!isset($php525)) {
    $php525 = version_compare(PHP_VERSION, '5.2.5', '>=');
  }

  if ($php525) {
    return htmlspecialchars($text, ENT_QUOTES, 'UTF-8');
  }

  return (preg_match('/^./us', $text) == 1) ? htmlspecialchars($text, ENT_QUOTES, 'UTF-8') : '';
}

/**
 * 检查字符串是否为 utf8 编码
 * @param (string) $text
 *  待检查的字符串
 * @return (bool)
 */
function dd_validate_utf8($text) {
  if (strlen($text) == 0) {
    return true;
  }
  return (preg_match('/^./us', $text) == 1);
}

/**
 * 字符串 url 编码
 * @param (string) $text
 *  待编码的字符串
 * @return (string) 编码后的字符串
 */
function dd_urlencode($text) {
  if (!is_string($text)) return $text;
  if ($GLOBALS['conf']['clean_url']) {
    return str_replace(array('%2F', '%26', '%23', '//'), array('/', '%2526', '%2523', '/%252F'), rawurlencode($text));
  } else {
    return str_replace('%2F', '/', rawurlencode($text));
  }
}

/**
 * 检查 uri 是否合法
 * @param (string) $uri
 *  待检查的 uri
 * @return (bool)
 */
function check_url($uri) {
  return filter_xss_bad_protocol($uri, false);
}

// public menu_xss_filter($value) {{{ 
/**
 * menu_xss_filter
 *  过滤尖括号，防止页面注入
 *  $_GET、$_SERVER['REQUEST_URI'] 默认将被过滤，若要获取未过滤的值，请访问全局变量 $_DGET
 *  例如：$_GET['q'] => $_DGET['q'], $_SERVER['REQUEST_URI'] => $_SERVER['_REQUEST_URI']
 * 
 * @param string|array $value
 * @access public
 * @return string
 */
function menu_xss_filter($value) {
  if (is_array($value)) {
    foreach ($value as $key => $v) {
      $value[$key] = menu_xss_filter($v);
    }
  } else {
    $value = strtr($value, array('<' => '%3C', '>' => '%3E'));
  }
  return $value;
}
// }}}

/**
 * 检查 uri 协议是否合法
 * @param (string) $string
 *  待检查的 uri
 * @param (bool) $decode
 *  是否需要解码
 * @return (bool)
 */
function filter_xss_bad_protocol($string, $decode = true) {
  static $allowed_protocols;

  if (!isset($allowed_protocols)) {
    $allowed_protocols = array_flip(var_get('filter_allowed_protocols', array('http', 'https', 'ftp', 'news', 'nntp', 'telnet', 'mailto', 'irc', 'ssh', 'sftp', 'webcal', 'rtsp')));
  }

  if ($decode) {
    $string = decode_entities($string);
  } do {
    $before = $string;
    $colonpos = strpos($string, ':');
    if ($colonpos > 0) {
      $protocol = substr($string, 0, $colonpos);
      if (preg_match('![/?#]!', $protocol)) {
        break;
      }
      if (!isset($allowed_protocols[strtolower($protocol)])) {
        $string = substr($string, $colonpos + 1);
      }
    }
  } while ($before != $string);

  return check_plain($string);
}

/**
 * 多维数组，按 weight 排序
 */
function dd_sort_cmp($a, $b) {
  
  $a_weight = isset($a['weight']) ? $a['weight'] : 0;
  $b_weight = isset($b['weight']) ? $b['weight'] : 0;
  
  if ($a_weight == $b_weight) {
    return 0;
  }
  
  return ($a_weight < $b_weight) ? -1 : 1;
}

/**
 * 字母数字混合模式的随机数
 * @return (string)
 */
function dd_range_str() {
  $a = range(a, z);
  $b = range(A, Z);
  return $a[rand(0, 25)]  . rand(1, 99) . $b[rand(0, 25)]. rand(99, 999);
}

/**
 * 默认 header
 */
function dd_page_header() {
  header("Expires: Thu, 9 Sep 1999 05:00:00 GMT");
  header("Last-Modified: ". gmdate("D, d M Y H:i:s") ." GMT");
  header("Cache-Control: store, no-cache, must-revalidate");
  header("Cache-Control: post-check=0, pre-check=0", false);
}

/**
 * 缓存页面 header
 * @param (int) $timestamp
 *  上次缓存时间
 * @return (bool) true
 */
function dd_page_cache_header($timestamp) {

  if (var_get('site_mode', 1)) {
    // 开发模式，不允许浏览器缓存
    dd_page_header();
    return;
  }

  $last_modified = gmdate('D, d M Y H:i:s', $timestamp) .' GMT';
  $etag = '"'. $timestamp .'"';

  $if_modified_since = isset($_SERVER['HTTP_IF_MODIFIED_SINCE']) ? $_SERVER['HTTP_IF_MODIFIED_SINCE'] : FALSE;
  $if_none_match = isset($_SERVER['HTTP_IF_NONE_MATCH']) ? stripslashes($_SERVER['HTTP_IF_NONE_MATCH']) : FALSE;

  if ($if_modified_since && $if_none_match
  && strpos($if_none_match, $etag) !== false
  && $if_modified_since == $last_modified) {

    // 客户端版本有效

    header('HTTP/1.1 304 Not Modified');
    header("Etag: $etag");

    exit;
  }

  header("Last-Modified: $last_modified");
  header("ETag: $etag");

  header("Expires: ".gmdate('D, d M Y H:i:s', ($_SERVER['REQUEST_TIME']+864000))." GMT");
  header("Cache-Control: must-revalidate");

  return true;
}

/**
 * 设置 meta 参数
 * @param (array) $v
 *  meta 值对
 */
function dd_set_meta(array $v = array()) {
  if (isset($v)) {
    if ($v['title']) {
      dd_set_title(array($v['title']));
      unset($v['title']);
    }
    foreach ($v as $key => $content) {
      if ($content) {
        dd_set_html_head("<meta name=\"$key\" content=\"$content\" />");
      }
    }
  }
}

/**
 * 获取分享网站代码，触发 hook_share_fav
 * @param (string) $url
 *  待分享的链接
 * @param (string) $title
 *  待分享的标题
 * @param (string) $desc
 *  描述
 * @param (string) $tags
 *  标签
 * @param (string) $pic
 *  图片地址，图片分享
 * @return (string) 所有分享网站链接列表
 */
function dd_get_share_fav($url, $title, $desc = '', $tags = '', $pic = '') {

  if ($lists = module_invoke_all('share_fav')) {
    $args = array(
      '<url>' => urlencode($url), '<title>' => urlencode($title),
      '<desc>' => urlencode($desc), '<tags>' => urlencode($tags),
      '<pic>' => $pic ? urlencode($pic) : ''
    );
    foreach ($lists as $key => $list) {
      if ($list['href']) {
        $link = strtr($list['href'], $args);
        if ($list['icon']) {
          $data = l(img($list['icon'], $list['title'], $list['title']), $link, array('html' => true,
          'attributes' => array('target' => '_blank', 'title' => $list['title'])));
        } else {
          $data = l($list['title'], $link, array('attributes' => array('target' => '_blank')));
        }
        $items[] = array(
          'data' => $data,
          'class' => 'share_'.$key
        );
      }
    }
    return theme('item_list', $items, NULL, 'ul', array('class' => 'share_fav_view_list'));
  }
}

/**
 * 将路径解析为数组，以 / 分割，从 0 开始
 * @param (int) $index
 *  索引，从 0 开
 * @param (string) $path
 *  路径，默认为 $_GET['q']
 */
function arg($index = NULL, $path = NULL) {
  static $arguments;
  if (!isset($path)) {
    $path = $_GET['q'];
  }
  if (!isset($arguments[$path])) {
    $arguments[$path] = explode('/', $path);
  }
  if (!isset($index)) {
    return $arguments[$path];
  }
  if (isset($arguments[$path][$index])) {
    return $arguments[$path][$index];
  }
}

/**
 * 将多维数组转为一维
 * @param (string) $op
 *  是保留索引，还是值，默认保留值
 * @param (array) $multi
 *  待转换多维数组
 * @param (array) &$array
 *  转换后的一维数组
 * @return (array)
 */
function dd_array2_to($op = 'value', $array, &$arr = array()) {
  foreach ($array as $key => $val) {
    if (is_array($val)) {
      dd_array2_to($op, $val, $arr);
    } else {
      $arr[] = $op == 'value' ? $val : $key;
    }
  }
  return $arr;
}

/**
 * 判断当前页是否为首页
 * @return (bool)
 */
function dd_is_front() {
  return strpos($_SERVER['PHP_SELF'], 'index.php') !== false
  && (!isset($_GET['q']) || $_GET['q'] == $GLOBALS['conf']['front']);
}

/**
 * 获取跳转地址
 * @param (string) $url
 * @return (string)
 *
 * 注：将废除，请使用 dd_get_redirect()
 */
function dd_redirect($url = NULL) {
  return array('query' => 'redirect='. ($url ? $url : url($_GET['q'])));
}

/**
 * 获取跳转地址，用于 url()
 * @param (string) $url
 *  指定地址，默认为 $_GET['q']
 * @return (string)
 */
function dd_get_redirect($url = NULL) {
  return array('query' => 'redirect='. urlencode(($url ? $url : url($_GET['q']))));
}

/**
 * 跳转至 redirect 或 指定路径
 * @param (string) $url
 */
function dd_goto_redirect($url = NULL) {
  dd_goto(!empty($_GET['redirect']) ? $_GET['redirect'] : $url);
}

/**
 * utf-8 字符转 ascii 编码
 * @param (string) $str
 *  待转换字符串
 * @param (string) $imp
 *  分隔符
 * @return (string)
 */
function dd_encode_ascii($str, $imp = '.') {
  if (!$str) return false;
  static $cache;
  if (!isset($cache[$str])) {
    $len = strlen($str);
    for ($i = 0; $i < $len; ++$i) {
      $ascii = ord(substr($str, $i, 1));
      if ($ascii > 128) {
        ++$i;
        $ascii .= ord(substr($str, $i, 1));
        ++$i;
        $ascii .= ord(substr($str, $i, 1));
      }

      $data[] = dechex($ascii);
    }
    $cache[$str] = implode($imp, $data);
  }
  return $cache[$str];
}

/**
 * ascii 编码的字符串转 utf-8 字符串
 * @param (string) $str
 *  待转换字符串
 * @param (string) $imp
 *  分隔符
 * @return (string)
 */
function dd_decode_ascii($str, $imp = '.') {
  if (!$str) return false;
  static $cache;
  if (!isset($cache[$str])) {
    $array = explode($imp, $str);
    foreach ($array as $temp) {
      $temp = hexdec($temp);
      $len = strlen($temp);
      for ($i = 0;$i < $len;$i += 3) {
        $cache[$str] .= chr(substr($temp, $i, 3));
      }
    }
  }
  return $cache[$str];
}

/**
 * 写入 php 文件 
 * @param (string) $filename
 *  文件名
 * @param (string) $var_name
 *  变量名
 * @param (*) $value
 *  待保存的数据
 * @param (string) $path
 *  保存路径，默认保存在缓存目录
 * @return bool|string 失败返回 false，成功返回文件路径
 */
function cache_system_set_file($filename, $var_name, $value, $path = 1) {
  $info = '//$Id$' . "\n\n";

  if (!is_string($value)) {
    $value = var_export($value, true);
    if (strpos($value, 'stdClass::__set_state') !== false) {
      $value = str_replace('stdClass::__set_state', '(object)', $value);
    }
    $value .= ';';
  }

  if ($path == 1) {
    $filepath = DIDA_ROOT . '/' . $GLOBALS['conf_dir'] . '/cache/' . $filename;
  } else {
    $filepath = DIDA_ROOT . '/' . $path . '/' . $filename;
  }

  if ($var_name) {
    $value = '$'. $var_name .' = '. $value;
  }

  if (file_put_contents($filepath, "<?php \n $info" . $value . "\n\n?>")) {
    return $filepath;
  } else {
    return false;
  }
}

/**
 * 从缓存目录获取 php 文件
 * @param (string) $filename
 *  文件名
 * @param (string) $var_name
 *  变量名
 */
function cache_system_get_file($filename, $var_name) {
  if (is_file(DIDA_ROOT . '/' . $GLOBALS['conf_dir'] . '/cache/' . $filename)) {
    include_once DIDA_ROOT . '/' . $GLOBALS['conf_dir'] . '/cache/' . $filename;
    return ${$var_name};
  }
}

/**
 * 删除缓存目录的文件
 * @param (string) $filename
 *  文件名称
 * @return (bool)
 */
function cache_del_file($filename) {
  if (strpos($filename, '/') === false) {
    $filename = DIDA_ROOT . '/' . $GLOBALS['conf_dir'] . '/cache/' . $filename;
  }

  return @unlink($filename);
}

/**
 * 必需项转为文字
 * @param (bool) $required
 * @return (string or null)
 */
function dd_get_required($required) {
  if ($required) return t('system', '是');
}

/**
 * 获取一条临时记录
 * @param (string) $tid
 *  记录 i
 * @return (object)
 */
function temp_get($tid) {
  $temp = db_query('SELECT data, type, timestamp, uid, serialized FROM {temp} WHERE tid = ?',
  array($tid), array('return' => 'one'));

  if (isset($temp->data)) {
    if ($temp->serialized) {
      $temp->data = unserialize($temp->data);
    }
    return $temp;
  }

  return 0;
}

/**
 * 写入一条临时记录
 * @param (string) $tid
 *  记录 i
 * @param (*) $data
 *  待写入数据
 * @param (string) $type
 *  自定义类型
 * @param (int) $uid
 *  用户 id，默认为当前用户
 * @return (bool)
 */
function temp_set($tid, $data, $type = '', $uid = NULL) {
  $v = array(
    'serialized' => 0,
    'data' => $data,
    'tid' => $tid,
    'timestamp' => $_SERVER['REQUEST_TIME'],
    'type' => $type,
    'uid' => isset($uid) ? $uid : $GLOBALS['user']->uid
  );

  if (is_object($data) || is_array($data)) {
    $v['data'] = serialize($data);
    $v['serialized'] = 1;
  }

  return db_replace('temp', $v);
}

/**
 * 删除一条或多条临时记录
 * @param (string) $tid
 * @return (int) 删除的列数
 */
function temp_del($tid) {
  return db_exec('DELETE FROM {temp} WHERE tid = ?', array($tid));
}

/**
 * 生成摘要
 * @param (string) $body
 *  正文
 * @param (int) $size
 *  摘要长度
 * @param (int) $format
 *  输入格式 id
 */
function dd_get_summary($body, $size, $format = NULL) {
  // 输入格式中有 PHP 过滤器
  if (!isset($format) && filter_is_php($format)) {
    return $body;
  }

  // 包含截断标志，优先
  if ($e = strpos($body, '<!-- break -->')) {
    $body = mb_substr($body, 0, $e);
    $size = mb_strlen(strip_tags($body));
    $body = $body .'</span>';
  }

  $_size = mb_strlen($body, 'utf-8');

  if ($_size <= $size) return $body;

  $strlen_var = strlen($body);

  // 不包含 html 标签
  if (strpos($body, '<') === false) {
    return mb_substr($body, 0, $size);
  }

  // html 代码标记
  $html_tag = 0;

  // 摘要字符串
  $summary_string = '';

  /**
   * 数组用作记录摘要范围内出现的 html 标签
   * 开始和结束分别保存在 left 和 right 键名下
   * 如字符串为：<h3><p><b>a</b></h3>，假设 p 未闭合
   * 数组则为：array('left' => array('h3', 'p', 'b'), 'right' => 'b', 'h3');
   * 仅补全 html 标签，<? <% 等其它语言标记，会产生不可预知结果
   */
  $html_array = array('left' => array(), 'right' => array());
  for ($i = 0; $i < $strlen_var; ++$i) {
    if (!$size) {
      break;
    }

    $current_var = substr($body, $i, 1);

    if ($current_var == '<') {
      // html 代码开始
      $html_tag = 1;
      $html_array_str = '';
    } else if ($html_tag == 1) {
      // 一段 html 代码结束
      if ($current_var == '>') {
        /**
         * 去除首尾空格，如 <br /  > < img src="" / > 等可能出现首尾空格
         */
        $html_array_str = trim($html_array_str);

        /**
         * 判断最后一个字符是否为 /，若是，则标签已闭合，不记录
         */
        if (substr($html_array_str, -1) != '/') {

          // 判断第一个字符是否 /，若是，则放在 right 单元
          $f = substr($html_array_str, 0, 1);
          if ($f == '/') {
            // 去掉 /
            $html_array['right'][] = str_replace('/', '', $html_array_str);
          } else if ($f != '?') {
            // 判断是否为 ?，若是，则为 PHP 代码，跳过

            /**
             * 判断是否有半角空格，若有，以空格分割，第一个单元为 html 标签
             * 如 <h2 class="a"> <p class="a">
             */
            if (strpos($html_array_str, ' ') !== false) {
              // 分割成2个单元，可能有多个空格，如：<h2 class="" id="">
              $html_array['left'][] = strtolower(current(explode(' ', $html_array_str, 2)));
            } else {
              /**
               * * 若没有空格，整个字符串为 html 标签，如：<b> <p> 等
               * 统一转换为小写
               */
              $html_array['left'][] = strtolower($html_array_str);
            }
          }
        }

        // 字符串重置
        $html_array_str = '';
        $html_tag = 0;
      } else {
        /**
         * 将< >之间的字符组成一个字符串
         * 用于提取 html 标签
         */
        $html_array_str .= $current_var;
      }
    } else {
      // 非 html 代码才记数
      --$size;
    }

    $ord_var_c = ord($body{$i});

    switch (true) {
      case (($ord_var_c & 0xE0) == 0xC0):
        // 2 字节
        $summary_string .= substr($body, $i, 2);
        $i += 1;
        break;
      case (($ord_var_c & 0xF0) == 0xE0):

        // 3 字节
        $summary_string .= substr($body, $i, 3);
        $i += 2;
        break;
      case (($ord_var_c & 0xF8) == 0xF0):
        // 4 字节
        $summary_string .= substr($body, $i, 4);
        $i += 3;
        break;
      case (($ord_var_c & 0xFC) == 0xF8):
        // 5 字节
        $summary_string .= substr($body, $i, 5);
        $i += 4;
        break;
      case (($ord_var_c & 0xFE) == 0xFC):
        // 6 字节
        $summary_string .= substr($body, $i, 6);
        $i += 5;
        break;
      default:
        // 1 字节
        $summary_string .= $current_var;
    }
  }
  $summary_string .= '…';
  if ($html_array['left']) {
    /**
     * 比对左右 html 标签，不足则补全
     */

    /**
     * 交换 left 顺序，补充的顺序应与 html 出现的顺序相
     * 如待补全的字符串为：<h2>abc<b>abc<p>abc
     * 补充顺序应为：</p></b></h2>
     */
    $html_array['left'] = array_reverse($html_array['left']);

    foreach ($html_array['left'] as $index => $tag) {
      // 判断该标签是否出现在 right 中
      $key = array_search($tag, $html_array['right']);
      if ($key !== false) {
        // 出现，从 right 中删除该单元
        unset($html_array['right'][$key]);
      } else {
        // 没有出现，需要补全
        $summary_string .= '</'.$tag.'>';
      }
    }
  }
  return $summary_string;
}

/**
 * 获取 rss 格式的数据
 * @param (string) $items
 *  通常为通过 dd_rss_item 获取的 rss 元素
 * @param (array) $channel
 *  rss 属性
 * @param $args
 *  $items 之前自定义参数
 * @return (string)
 */
function dd_rss_feed($items, $channel = array(), $args = array()) {
  global $conf, $base_url;

  $namespaces = array(
    'xmlns:dc' => 'http://purl.org/dc/elements/1.1/',
    'xml:base' => $base_url
  );

  $channel_defaults = array('version' => '2.0', 'root' => 'channel');

  if ($channel['namespaces']) {
    $namespaces = array_merge($namespaces, $channel['namespaces']);
  }

  if ($channel) {
    $channel = array_merge($channel_defaults, $channel);
  } else {
    $channel = $channel_defaults;
  }

  $output = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  $output .= "<rss version=\"". $channel["version"] ."\" ". dd_attributes($namespaces) .">\n";
  $output .= "<$channel[root]>\n";

  if ($args) {
    $output .= dd_xml_elements($args);
  }

  $output .= $items;
  $output .= "</$channel[root]>\n";
  $output .= "</rss>\n";

  return $output;
}

/**
 * 数据转换为 xml 元素
 * @param (array or string) $fields
 *  待转换数据
 * @param (string) $item
 *  xml 标签
 * @param (array) $args
 *  自定义属性
 * @return (string)
 */
function dd_rss_item($fields, $item = 'item', $args = array()) {
  $output = "<$item>\n";
  if (is_array($fields)) {
    foreach ($fields as $key => $val) {
      if (!is_array($val)) {
        if (strpos($val, '<') !== false) {
          $val = '<![CDATA[ '.$val.' ]]> ';
        }
        $output .= "<$key>". $val ."</$key>\n";
      } else {
        if (!is_numeric($key)) {
          $output .= "<$key>";
          $output .= dd_xml_elements($val, $key);
          $output .= "</$key>";
        } else {
          $output .= dd_xml_elements($val, $key);
        }
      }
    }
  } else if ($fields) {
    $output .= $fields;
  }

  if ($args) $output .= dd_xml_elements($args);

  $output .= "</$item>\n";
  return $output;
}

/**
 * 解析 xml 自定义属性
 * @param (array) $array
 *  待解析数组
 * @param (string) $root
 *  项目名称
 * @return (string)
 */
function dd_xml_elements($array, $root = '') {
  $output = '';
  foreach ($array as $key => $value) {
    if (is_numeric($key)) {
      if ($root) {
        $output .= ' <'. $root;
        if (isset($value['attributes']) && is_array($value['attributes'])) {
          $output .= dd_attributes($value['attributes']);
        }
        if ($value['value'] != '') {
          $output .= '>'. (is_array($value['value']) ? dd_xml_elements($value['value']) : check_plain($value['value'])) .'</'. $root .">\n";
        } else {
          $output .= " />\n";
        }
      }
    } else if (!is_numeric($key)) {
      $output .= ' <'. $key .'>' . (is_array($value) ? dd_xml_elements($value) : check_plain($value)) ."</$key>\n";
    }
  }
  return $output;
}


/**
 * 默认 feed 输
 * @param (string) $items
 *  通常为使用 dd_rss_item() 获取的 xml 元素
 * @param (string) $title
 *  标题
 * @param (string) $link
 *  链接
 * @param (string) $des
 *  描述
 */
function dd_feed_wrapper($items, $title, $link = NULL, $des = NULL) {
  global $conf, $base_url;

  if (strpos($conf['site_global']['logo'], 'http') === false) {
    $logo = $base_url . $conf['site_global']['logo'];
  } else {
    $logo = $conf['site_global']['logo'];
  }

  return dd_rss_feed($items, NULL, array(
    'title' => $title .'_'. $conf['site_global']['name'],
    'description' => ($des ? $des : $conf['site_global']['description']),
    'link' => ($link ? $link : $base_url),
    'image' => array(
      'url' => $logo,
      'title' => $conf['site_global']['name'],
      'link' => $base_url,
  ),
  ));
}

// public dd_in_array($value, array $array) {{{ 
/**
 * dd_in_array
 *  检查数组中是否存在某个值。in_array() 在检查 int 类型的值时会出现异常，可使用本函数替代
 * 
 * @param string $value 
 * @param array $array 
 * @access public
 * @return bool
 */
function dd_in_array($value, array $array) {
  foreach ($array as $v) {
    if ($value == (string) $v) {
      return true;
    }
  }
  return false;
}
// }}}
