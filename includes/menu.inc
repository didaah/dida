<?php
// $Id$

define('MENU_NOT_FOUND', 1);//404错误
define('MENU_ACCESS_DENIED', 2);//403错误
define('MENU_ACCESS_OK', 5);//检查url权限

// public menu_get_item($path = NULL) {{{ 
/**
 * menu_get_item
 *  根据 path 获取内容，path 由 hook_menu() 定义
 * 
 * @param string $path 
 *  默认为 $_GET['q']
 * @access public
 * @return void
 */
function menu_get_item($path = NULL) {
  global $user, $conf, $current_menu_data;
  static $access;
  if (!$path) {
    $url = $_GET['q'];
    $get_view = true;
  } else {
    //检查当前用户是否有访问给定路径的权限
    if (isset($access[$path])) {
      return $access[$path];
    } else {
      $access[$path] = false;
    }
    $url = $path;
    $get_view = false;
  }
  
  if (empty($url)) {
    return front_page();
  } else {
    if (substr($url, -1) == '/') {
      $url = substr($url, 0, -1);
    }

    if (!$path && $conf['alias'] && ($alias = menu_get_alias_path($url, '*'))) {
    	dd_goto($alias, dd_query_string_encode($_REQUEST, array_merge(array('q'), array_keys($_COOKIE))));
    	exit;
    }
    
    $systemUrl = $cacheUrl = $url;

    // 防止 url 过长
    $cacheUrl = md5($cacheUrl);

    $item = NULL;
    
    if ($get_view) {
      if ($current_menu_data && $current_menu_data['systemUrl']) {
        $item = $current_menu_data;
      } else if ($cache = cache_get($cacheUrl, 'cache_menu')) {
        $current_menu_data = $item = $cache->data;
      }
    } else if ($cache = cache_get($cacheUrl, 'cache_menu')) {
      $item = $cache->data;
    }
      
    if (empty($item)) {
      if ($toPath = menu_get_alias_path($url, 'alias')) {
      	$systemUrl = $url = $toPath;
      }

      $t = arg(NULL, $url);
      
      $parts = array_slice($t, 0, 10);
      
      $array = menu_get_val($parts);
      
      if ($item = db_query('SELECT * FROM {menu} WHERE path IN ('. implode (',', $array[1]) .') ORDER BY fit DESC', $array[0], array('return' => 'one', 'fetch' => 'array', 'limit' => 1))) {
      	$item['systemUrl'] = $systemUrl;
      	if ($cacheUrl != $systemUrl) {
      	  $item['aliasUrl'] = $cacheUrl;
      	}

        // 缓存 1 周
        cache_set($cacheUrl, $item, 'cache_menu', 604800);
        
        // 当前路径信息
        if (!empty($get_view)) {
          $current_menu_data = $item;
        }
      } else {
        cache_set($cacheUrl, MENU_NOT_FOUND, 'cache_menu', 604800);
        $item = MENU_NOT_FOUND;
      }
    }
  }
  
  if ($item) {
    // 未定义的路径
    if ($item == MENU_NOT_FOUND) {
      if ($path) {
        $access[$path] = false;
        return $access[$path];
      }
      return MENU_NOT_FOUND;
    }
    
    /**
     * 如果当前路径是别名，则将 $_GET['q'] 重写为真实路径
     * 即使用 $_GET['q'] 获取到的永远是系统路径，可能和实际浏览器中显示的地址不一样
     * 将实际访问的 q 参数，赋值到 $_GET['__q'];
     */
    $_GET['__q'] = $_GET['q'];
    $_GET['q'] = $item['systemUrl'];   

    $page_args = $title_args = array();
    
    if ($item['load_functions']) {
      $load = menu_match(unserialize($item['load_functions']), $item['systemUrl']);
      if ($load === false) return MENU_NOT_FOUND;
    } else {
      $load = arg(NULL, $url);
    }
    if ($item['access_callback'] != 1) {
      if ($item['access_arguments']) {
        if ($load) {
          foreach (unserialize($item['access_arguments']) as $arg) {
            if ($load[$arg]) {
              $args[] = $load[$arg];
            } else {
              $args[] = $arg;
            }
          }
        } else {
          $args = unserialize($item['access_arguments']);
        }
        
        if (!call_user_func_array($item['access_callback'], $args)) {
          return MENU_ACCESS_DENIED;
        }
        
      } else if (!call_user_func($item['access_callback'])) {
        return MENU_ACCESS_DENIED;
      }
    }
    
    if ($path) {
      $access[$path] = MENU_ACCESS_OK;
      return $access[$path];
    }
    
    if (!empty($item['file'])) {
      require_once DIDA_ROOT . '/' . $item['file'];
    }
    
    if (!empty($item['page_arguments'])) {
      $page_args = array();
      if ($load) {
        foreach (unserialize($item['page_arguments']) as $arg) {
          if ($load[$arg]) {
            $page_args[] = $load[$arg];
          } else {
            $page_args[] = $arg;
          }
        }
      } else {
        $page_args = unserialize($item['page_arguments']);
      }
    }
    
    if (!empty($item['title_callback'])) {
      $title_args = array();
      if ($item['title_arguments']) {
        if ($load) {
          foreach (unserialize($item['title_arguments']) as $arg) {
            if ($load[$arg]) {
              $title_args[] = $load[$arg];
            } else {
              $title_args[] = $arg;
            }
          }
        } else {
          $title_args = unserialize($item['title_arguments']);
        }
      }
      $title = call_user_func_array($item['title_callback'], $title_args);
    } else if (!empty($item['title'])) {
      $title = t($item['module'], $item['title']);
    } else {
      $title = '';
    }
 
    if (!empty($item['page_template'])) {
      $template_args = array();
      if (!empty($item['template_arguments'])) {
        if ($load) {
          foreach ($item['template_arguments'] as $key => $arg) {
            if ($load[$arg]) {
              $template_args[$key] = $load[$arg];
            } else {
              $template_args[$key] = $arg;
            }
          }
        } else {
          $template_args = $item['template_arguments'];
        }
      }
      $template_file = array(
        DIDA_ROOT . $conf['base_theme'] . '/' . $item['module'] . '/' . $item['page_template'],
        DIDA_ROOT . '/' . $conf['modules'][$item['module']]['path'] . '/tpl/' . $item['page_template'],
      );
    }
   
    if (!empty($title)) dd_set_title(array($title));
     
    $output = '';

    if (!empty($item['page_callback'])) $output = call_user_func_array($item['page_callback'], $page_args);
    if (!empty($template_file)) {
      foreach ($template_file as $tpl) {
        if (is_file($tpl)) {
          /**
           * 可在 tpl 中使用 $output，该变量为 page_callback 函数所返回数据
           */
          $template_args['output'] = $output;
          $output = theme_render_tpl($tpl, $template_args);
          break;
        }
      }
    }

    if (!empty($_POST) && !empty($_POST['_dida_ajax_return_body'])) {
      echo $output;
      exit;
    }
    return $output;
  } else {
    return MENU_NOT_FOUND;
  }
}
// }}}

// 根据系统路径删除别名
function menu_delete_path($path) {
  if (db_exec('DELETE FROM {alias} WHERE path = ?', array($path)) && 
  $alias = db_query('SELECT alias FROM {alias} WHERE path = ?', array($path), array('return' => 'column'))) {
    // 清除 menu 缓存
    cache_del('cid', md5($alias), 'cache_menu');
  }
}

// 根据别名删除
function menu_delete_alias($alias) {
  db_exec('DELETE FROM {alias} WHERE alias = ?', array($alias));
  // 清除 menu 缓存
  cache_del('cid', md5($alias), 'cache_menu');
}

// 写入别名
function menu_set_alias($path, $alias, $type, $tid) {
  $alias = trim($alias);
  $path = trim($path);
  if (!db_query('SELECT alias FROM {alias} WHERE alias = ?', array($alias), array('return' => 'column'))) {
    // 清除 menu 缓存
    cache_del('cid', md5($path), 'cache_menu');
    cache_del('cid', md5($alias), 'cache_menu');
    return db_exec('INSERT INTO {alias} (path, alias, type, tid) VALUES (?, ?, ?, ?)', array($path, $alias, $type, $tid));
  }
}

// 更新别名
function menu_update_alias($alias, $oldalias) {
  $alias = trim($alias);
  if (db_exec('UPDATE {alias} SET alias = ? WHERE alias = ?', array($alias, $oldalias)) && $path = db_query('SELECT path FROM {alias} WHERE alias = ?', array($alias), array('return' => 'column'))) {
    cache_del('cid', md5($oldalias), 'cache_menu');
    return $path;
  }
}

//查询别名和系统路径
function menu_get_alias_path($str, $op = 'alias') {
  // $alias: $path => $alias, $path: $alias => $path
	static $alias, $path;
  switch ($op) {
    case 'alias': // 根据别名查询系统路径
  		if (!isset($path[$str])) {
      	if ($path[$str] = db_query('SELECT path FROM {alias} WHERE alias = ?', array($str), array('limit' => 1, 'return' => 'column'))) {
      		$alias[$path[$str]] = $str;
      	}
    	}
    break;
    case 'path':
      if (!isset($alias[$str])) {
      	// 根据系统路径查询别名
      	if ($alias[$str] = db_query('SELECT alias FROM {alias} WHERE path = ?', array($str), array('limit' => 1, 'return' => 'column'))) {
      		$path[$alias[$str]] = $str;
      	}
      }
    break;
    case '*': // 路径或别名
      if ($o = db_query('SELECT alias, path FROM {alias} WHERE path = ? OR alias = ?', array($str, $str), array('limit' => 1, 'return' => 'one'))) {
        if ($o->alias == $str) {
          // 别名等于 $str
          $path[$str] = $o->path;
          $alias[$o->path] = $str;
        } else {
          $alias[$str] = $o->alias;
          $path[$o->alias] = $str;
        }
      } else {
        $alias[$str] = $path[$str] = false;
      }
  }
  return $op == 'alias' ? $path[$str] : $alias[$str];
}


//根据系统路径获取别名
function menu_get_alias($path) {
  static $alias;
  if (!isset($alias[$path])) {
    $alias[$path] = db_query('SELECT alias FROM {alias} WHERE path = ?', array($path), array('limit' => 1, 'return' => 'column'));
  }
  return $alias[$path];
}

// 根据别名获取系统路径
function menu_get_path($alias) {
  static $path;
  if (!isset($path[$alias])) {
    $path[$alias] = db_query('SELECT path FROM {alias} WHERE alias = ?', array($alias), array('limit' => 1, 'return' => 'column'));
  }
  return $path[$alias];
}

// 检查保留路径
function menu_check_reservation($alias) {
	if ($lists = module_invoke_all('alias_reservation')) {
		return _menu_check_path_filter($alias, $lists);
  }
}

/**
 * 比对路径是否在给定的数组中，/ 为目录匹配，即 user/ 匹配 user 下所有路径
 * @param $menu = String
 *  要检查的路径
 * @param $lists = array()
 *  包含一组路径
 * @return
 *  true 或 false：存在 或 不存在
 */
function _menu_check_path_filter($menu, $lists = array()) {
	foreach ($lists as $path) {
    if ($path == '<front>') {
      return dd_is_front();
    }
    if (substr($path, -1) == '/') {
      $path = substr($path, 0, -1);
      if (strpos($path, '/') === false) {
      	if (arg(0, $menu) == $path) {
      		return true;
      	}
      } else {
      	$args = arg(NULL, $menu);
      	$reservation = true;
      	foreach (explode('/', $path) as $id => $arg) {
      		if ($args[$id] != $arg) {
      			$reservation = false;
      			break;
      		}
      	}
      	if ($reservation) {
      		return true;
      	}
      }
    } else if ($path == $menu) {
    	return true;
    }
  }
}

//查询定义当前url的模块
function menu_get_val($parts) {
  $number_parts = count($parts);
  $placeholders = array();
  $ancestors = array();
  $length =  $number_parts - 1;
  $end = (1 << $number_parts) - 1;
  $masks = var_get('menu_masks', array(), 'variable');
  foreach ($masks as $i) {
    if ($i > $end) {
      continue;
    } elseif ($i < (1 << $length)) {
      --$length;
    }
    $current = '';
    for ($j = $length; $j >= 0; $j--) {
      if ($i & (1 << $j)) {
        $current .= $parts[$length - $j];
      }
      else {
        $current .= '%';
      }
      if ($j) {
        $current .= '/';
      }
    }
    $reg[] = '?';
    $val[] = $current;
  }
  return array($val, $reg);
}

function menu_match($load, $path = NULL) {
  $return = array();
  foreach ($load as $key => $match) {
    if (!is_array($match)) {
      if (!$return[$key] = menu_menu_match(arg($key, $path), $match)) {
        return false;
      }
    } else {
    	$function = $match[0];
      foreach ($match as $id => $value) {
        if ($id == 0) {
          $match[0] = arg($key, $path);
        } else if (arg($value, $path)) {
          $match[$id] = arg($value, $path);
        }
      }
    	if (!$return[$key] = call_user_func_array($function, $match)) {
        return false;
      }
    }
  }
  return $return;
}

/**
 * menu 占位符
 * @param (string) $str
 *  arg(x)
 * @param (string) $match
 *  占位符
 * @return (*) 替换后的数据 
 */
function menu_menu_match($str, $match) {
  switch ($match) {
    case '%':
    return (string) $str;
    case '%0-9':
      if (!preg_match('/[^0-9]/', $str)) return (int) $str;
    break;
    case '%a-z':
      if (!preg_match('/[^a-z]/i', $str)) return $str;
    break;
    case '%0-9html':
      if (strpos($str, '.') !== false) {
        $h = explode('.', $str, 2);
        if ($h[1] == 'html' && !preg_match('/[^0-9]/', $h[0])) {
          return (int) $h[0];
        }
      }
    break;
    case '%html':
      if (strpos($str, '.') !== false) {
        $h = explode('.', $str, 2);
        if ($h[0] && $h[1] == 'html') {
          return (string) $h[0];
        }
      }
    break;
    case '%0-9shtml':
      if (strpos($str, '.') !== false) {
        $h = explode('.', $str, 2);
        if ($h[1] == 'shtml' && !preg_match('/[^0-9]/', $h[0])) {
          return (int) $h[0];
        }
      }
    break;
    case '%shtml':
      if (strpos($str, '.') !== false) {
        $h = explode('.', $str, 2);
        if ($h[0] && $h[1] == 'shtml') {
          return (string) $h[0];
        }
      }
    break;
  }
  return false;
}

/**
 * 写入所有模块定义 menu
 */
function menu_set_item() {
  $menus = array();
  $modules = $GLOBALS['conf']['modules'];
  foreach ($modules as $module => $info) {
    $function = $module .'_menu';
    if (function_exists($function) && $result = call_user_func($function)) {
      if (!empty($result) && is_array($result)) {
        foreach ($result as $path => $menu) {
          /**
           * 重写 menu 的定义
           * hook_menu_alter($path, $menu)
           */
          module_alter_all('menu_alter', $path, $menu);

          $t = array();
          $fit = 0;
          $path = trim($path);

          if (empty($path)) {
            dd_set_message(t('system', '%module 模块定义了一个空路径，跳过', array('%module' => $module)), 'warning');
            continue;
          }

          $arg = explode('/', $path, 10);

          if ($arg[0] == '%') {
            dd_set_message(t('system', '%module 模块定义的路径 %path 出现错误，不允许在 url 第一级使用通配符，跳过',
            array('%module' => $module, '%path' => $path)), 'warning');
            continue;
          }

          if (!$menu['page_callback']) {
            dd_set_message(t('system', '%module 模块定义的路径 %path 没有提供回调函数，跳过',
            array('%module' => $module, '%path' => $path)), 'warning');
            continue;
          }
          
          $number_parts = count($arg);
          $slashes = $number_parts - 1;
          foreach ($arg as $k => $part) {
            if ($part != '%') {
              $fit |=  1 << ($slashes - $k);
            }
          }

          if (!$fit) {
            $fit = (1 << $number_parts) - 1;
          }

          $masks[$fit] = 1;
          $sort[$path] = $number_parts;
          
          $menu['fit'] = $fit ? $fit : 0;
          $menu['module'] = $module;

          if (!empty($menu['load_functions'])) {
            $menu['load_functions'] = serialize($menu['load_functions']);
          }

          if (!empty($menu['file'])) {
            // 如果不是绝对路径
            if (!is_file($menu['file'])) {
              $menu['file'] = $modules[$module]['path'] . '/' . $menu['file'];
            }
          }

          if (!empty($menu['title_arguments'])) {
            $menu['title_arguments'] = serialize($menu['title_arguments']);
          }

          if (!empty($menu['access_arguments'])) {
            $menu['access_arguments'] = serialize($menu['access_arguments']);
          }

          if (!empty($menu['page_arguments'])) {
            $menu['page_arguments'] = serialize($menu['page_arguments']);
          }

          $menu['access_callback'] = !empty($menu['access_callback']) ? $menu['access_callback'] : 'user_access';
          
          if (empty($menu['weight'])) {
            $menu['weight'] = 0;
          }
          
          if (empty($menu['title_callback'])) {
            $menu['title_callback'] = '';
          }
          
          if (empty($menu['description'])) {
            $menu['description'] = '';
          }
          
          $menu['path'] = $path;
          
          $menus[$path] = $menu;
        }
      }
    }
  }
  
  if ($menus) {
    array_multisort($sort, SORT_NUMERIC, $menus);
    db_query('DELETE FROM {menu}');
    
    $fields = array(
      'path', 'module', 'title', 'title_callback', 'title_arguments', 'load_functions',
      'access_callback', 'access_arguments', 'page_callback', 
      'page_arguments', 'description', 'file', 'fit', 'weight'
    );
    
    $place = $args = array();
    
    $i = 0;
    foreach ($menus as $path => $menu) {
      $keys = array();
      foreach ($fields as $field) {
        $key = ':'.$field.$i;
        $args[$key] = $menu[$field];
        $keys[] = $key;
      }
      $place[] = '('.implode(',', $keys).')';
      ++$i;
    }
    
    db_exec('INSERT INTO {menu} ('.implode(',', $fields).') VALUES '.implode(',', $place), $args);
    
    $masks = array_keys($masks);
    rsort($masks);
    var_set('menu_masks', $masks);
  }
  return true;
}
