<?php
// $Id$

/**
 * @param object $ac
 *  用户对象
 * @return string
 */
function user_page_view($ac) {
  $type = arg(3);

  if (empty($type)) return dd_get_access();

  $ac->tabs = array();

  $info = module_invoke_all('user_page_info', $ac);

  if (empty($info[$type])) return dd_get_access();

  // 默认约定输出值
  $ac->content = '';

  module_alter_all('user_page_view', $ac, $type);

  dd_set_title(array($info[$type]['title']));

  return theme('user_page', $ac, $type);
}

/**
 * 跳转页
 */
function user_default() {
  global $user;
  if (!$user->uid) {
    dd_goto('user/login');
  } else {
    dd_goto('user/' . $user->uid);
  }
}

/**
 * @Implement of hook_user_center() 
 * @return array
 */
function user_user_center_info() {
  return array(
    'user' => array(
      'is_group' => true, // 定义组信息
      'weight' => 1000,
      'list_title' => t('user', '基本设置'), // 列表标题
      // 以下三个参数设置与 hook_menu() 相同，可用于控制权限、设置动态菜单，
      //'access_callback' => 'user_access', // 权限回调函数，默认为 user_access
      //'access_arguments' => array('user view'),
      //'title_callback' => '', // 标题回调函数
    ),
    'default' => array(
      'title' => t('user', '个人中心'),
      // 'page_title' => '我的个人中心', // 页面标题
      'link_hide' => true, // 不显示为链接
      'path' => 'user/center',
      'weight' => -1000
    ),
    'newpass' => array(
      'title' => t('user', '修改密码'),
    ),
    'edit' => array(
      'title' => t('user', '编辑资料')
    ),
    'logout' => array(
      'title' => t('user', '安全离开'),
      'weight' => 1000,
      'path' => 'user/logout'
    )
  );
}

/**
 * 个人中心页面显示
 * @Implement of hook_user_center_view() 
 * @param object $center 
 *  页面对象
 * @access public
 * @return void
 */
function user_user_center_view($center) {
  switch ($center->type) {
    case 'default':
      if ($data = _module_invoke_all('user_center_default', $center)) {
        $i = 0;
        $contents = array();
        foreach ($data as $module => $content) {
          $row = array();
          if (is_array($content)) {
            $row['#weight'] = !isset($content['#weight']) ? $i : $content['#weight'];
            $row['#data'] = $content['#data'];
          } else {
            $row['#weight'] = $i;
            $row['#data'] = $content;
          }

          if (empty($row['#data'])) continue;

          $row['#data'] = '<div class="user_center_default_view_' . $module . '">' . $row['#data'] . '</div>';
          
          $contents[] = $row;
          ++$i;
        }
        
        uasort($contents, 'dd_form_cmp');
        
        foreach ($contents as $d) {
          $center->body .= $d['#data'];
        }
      }
    break;
    case 'newpass':
      $center->body = dd_get_form('_user_center_newpass'); 
    break;
    case 'edit':
      $center->body = dd_get_form('user_edit_form', $center->user);
    break;
  }

}

/**
 * 获取个人中心页面 menu 
 * @param object $center
 *  个人中心对象
 * @access protected
 * @return array
 */
function _user_get_center_info($center) {
  global $language_code; 

  /**
   * 读取自定义设置
   * @TODO 目前提供自定义排序，未来提供自定义页面名称等设置
   */
  if ($cache = cache_get('user_center_info_settings')) {
    $group = $cache->data['group'];
    $links = $cache->data['links'];
  } else {
    $group = array('weight' => array(), 'list_title' => array(), 'access' => array());
    $links = array('weight' => array());
  }

  if ($functions = module_hook_get_cache('user_center_info')) {
    $i = 0;
    $menu = $_menu = array();
    foreach ($functions as $module => $function) {
      if (function_exists($function)) {
        $result = call_user_func($function);
        
        // hook_user_center_info() 必须返回数组
        if (!is_array($result)) continue;

        foreach ($result as $type => $info) {

          // 组默认权限
          if (!isset($group['access'][$type])) $group['access'][$type] = true;

          if (!empty($info['access_arguments'])) {
            $access_callback = !empty($info['access_callback']) ? $info['access_callback'] : 'user_access';
            if (!call_user_func_array($access_callback, $info['access_arguments'])) {
              if (!empty($info['is_group'])) $group['access'][$type] = false;
              continue;
            }
          } else if (!empty($info['access_callback']) && !call_user_func($info['access_callback'])) {
            if (!empty($info['is_group'])) $group['access'][$type] = false;
            continue;
          }

          // 定义组信息
          if (!empty($info['is_group'])) {
            foreach ($info as $key => $value) {
              $group[$key][$type] = $value;
            }
            continue;
          }

          if (empty($info['group'])) {
            $info['group'] = $module;
          }

          // 无权限
          if (empty($group['access'][$info['group']])) continue;

          if (!empty($info['title_callback'])) {
            $info['title'] = call_user_func($info['title_callback']);
          }
        
          // 未定义页面标题，跳过
          if (empty($info['title'])) continue;
          
          // 自定义页面排序
          if (isset($links['weight'][$type])) {
            $info['weight'] = $i+$links['weight'][$type];
          } else if (!empty($info['weight'])) {
            $info['weight'] += $i;
          } else {
            $info['weight'] = $i;
          }

          if (empty($menu[$info['group']])) {
            $menu[$info['group']] = array('links' => array());
            // 自定义分组排序
            if (isset($group['weight'][$info['group']])) {
              $menu[$info['group']]['weight'] = $i+$group['weight'][$info['group']];
            } else {
              $menu[$info['group']]['weight'] = $i;
            }
          }
          
          $class = 'user_center_link user_center_link_' . $info['group'];

          if (empty($info['attributes']) || empty($info['attributes']['class'])) {
            $info['attributes'] = array('class' => $class);
          } else {
            $info['attributes']['class'] .= ' ' . $class;
          }

          if (empty($info['path'])) {
            $info['path'] = 'user/center/' . $type;
          }

          $info['attributes']['id'] = 'user_center_link_menu_' . $type;

          $_menu[$type] = $info;

          // 只注册一个页面，不做为 link 显示
          if (!empty($info['link_hide'])) continue;

          $menu[$info['group']]['links'][$type] = $info;
          $menu[$info['group']]['list_title'] = !empty($group['list_title'][$info['group']]) ? $group['list_title'][$info['group']] : NULL;
          ++$i;
        }
      }
    }

    // 组排序
    uasort($menu, 'dd_sort_cmp');
   
    $data = array('menu' => $_menu, 'links' => array());

    foreach ($menu as $type => $link) {
      uasort($link['links'], 'dd_sort_cmp');
      $data['links'][$type] = $link;
    }
  }

  $center->menu = $data['menu'];
  $center->links = '';
  $center->groups = $group;

  foreach ($data['links'] as $type => $link) {
    $item = array();
    foreach ($link['links'] as $key => $info) {
      $item[] = array(
        'data' => l($info['title'], $info['path'], $info),
        'id' => 'user_center_link_item_list_' . $type . '_' . $key,
      );
    }
    $center->links .= theme('item_list', $item, $link['list_title'], 'ul', array(
      'class' => 'user_center_link_item user_center_link_item_wrapper_' . $type,
      'id' => 'user_center_link_item_' . $type
    ));
  }

}

/**
 * @Implement of hook_user_page()
 */
function user_user_center_default($center) {
  $item = array();
  $item[] = dd_get_strong(t('user', '注册时间')) . format_date($center->user->created);
  
  if (!empty($ac->login)) {
    $item[] = dd_get_strong(t('user', '最近登录')) . format_date($center->user->login);
  }

  $contents = '';
  
  if ($center->user->field_view) {
    foreach ($center->user->field_view as $key => $data) {
      if ($key != 'avatar') {
        $item[] = array('data' => dd_get_strong($data['#name']) . $data['#content'], 'class' => 'user_field_view_' . $key);
      } else {
        $contents .= '<div class="user_field_view_avatar">' . $data['#content'] . '</div>';
      }
    }
  }

  $contents .= theme('item_list', $item, $center->user->name, 'ul', array('id' => 'user_center_default_view_fields'));

  return array('#data' => $contents, '#weight' => -100);
}

/**
 * 用户个人中心
 */
function user_page_center($type = 'default') {
	global $user;

  $center = new stdClass;

  $center->body = '';

  $center->user = user_load($user->uid);
  
  $center->args = arg();

	$center->type = $type;

  $center->tabs = array();

  /**
   * 获取 $center->menu 和 $center->links
   * $center->menu 是一个数组，包含所有个人中心的注册页面，约等于 module_invoke_all('user_center_info') 
   * $center->links 是一个字符串，是经过处理的 $center->menu，一般用于模板输出
   */
  _user_get_center_info($center);

  if (empty($center->menu[$center->type])) {
    $center->type = 'default';
  }

  $title = array(t('user', '我的个人中心'));
  $breadcrumb = array(l(t('user', '我的个人中心'), 'user/center'));

  if (!empty($center->menu[$center->type]['group']) && !empty($center->groups['list_title'])) {
    $title[] = $breadcrumb[] = $center->groups['list_title'][$center->menu[$center->type]['group']];
  }

  if (!empty($center->menu[$center->type]['page_title'])) {
    $breadcrumb[] = $title[] = $center->menu[$center->type]['page_title'];
  } else {
    $breadcrumb[] = $title[] = $center->menu[$center->type]['title'];
  }

	dd_set_title(array_reverse($title));
	dd_set_breadcrumb($breadcrumb);
  
  module_invoke_all('user_center_view', $center);
  
  return theme('user_center', $center);
}

/**
 * 找回密码
 */
function user_page_pass() {
  switch ($_GET['type']) {
    case 'newpass':
      if ($_GET['str'] && $_GET['uid']) {
        if ($uid = db_query('SELECT uid FROM {users} WHERE uid = ? AND newpass = ? AND active > ?', array($_GET['uid'], $_GET['str'], ($_SERVER['REQUEST_TIME'] - 86400)), array('return' => 'column'))) {
          db_exec('UPDATE {users} SET newpass = 0 WHERE uid = ?', array($uid));
          if (user_login(user_load($uid))) {
            setcookie(session_name(), session_id(), -1, '/', $cookie_domain);
            $_SESSION['oldpass'] = time();
            dd_set_message(t('user', '请在 5 分钟内修改密码'));
            dd_goto('user/center/newpass');
          } else {
            dd_goto('user/pass');
          }
        }
      } else {
        dd_set_message(t('user', '错误的请求'), 'error');
      }
    break;
  }
  
  $form['settings'] = array(
    '#values_object' => true,
    '#validate' => array('user_page_pass_validate'),
    '#description' => t('user', '请输入注册邮箱或昵称'),
  );
  
  $form['fields']['mail'] = array(
    '#type' => 'textfield',
    '#title' => t('user', '邮箱'),
  );
  
  $form['fields']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('user', '昵称'),
  );
  
  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#submit' => 'user_page_pass_submit',
  );

  return $form;
}

function user_page_pass_validate(&$form, &$v) {
  if ($v['name']) {
    $v['_dida_values']->_ac = db_query('SELECT status, name, uid, mail FROM {users} WHERE name = ?',
    array($v['name']), array('return' => 'one'));
  } else if ($v['mail']) {
    $v['_dida_values']->_ac = db_query('SELECT status, name, uid, mail FROM {users} WHERE mail =?',
    array($v['mail']), array('return' => 'one'));
  } else {
    $t['mail']['#error'][] = t('user', '邮箱或昵称必填一项');
  }
  
  if (!isset($v['_dida_values']->_ac->uid)) {
    $t['mail']['#error'][] = t('user', '帐号或邮箱尚未注册');
  } else if ($v['_dida_values']->_ac->status == 0) {
    $t['mail']['#error'][] = t('user', '帐号还未激活，<a href="!link">立即激活</a>',
    array('!link' => url('user/activation')));
  }
  
  return $t;
}

function user_page_pass_submit(&$form, &$v) {
  if ($v['_dida_values']->_ac->uid) {
    if (_user_newpass($v['_dida_values']->_ac->uid, $v['_dida_values']->_ac->name, $v['_dida_values']->_ac->mail)) {
      dd_set_message(t('user', '找回密码的方式已发送到你的邮箱，请注意查收'));
    } else {
      dd_set_message(t('user', '找回失败，请重试'));
    }
  }
}

/**
 * 修改密码
 */
function _user_center_newpass() {
  global $user;
  
  if ($_GET['ajax'] == 'submit') {
    if (!$_POST['pass']) {
      $error[] = t('user', '新密码不能为空');
    } else if ($_POST['pass'] != $_POST['pass2']) {
      $error[] = t('user', '两次输入的新密码不一致');
    } else {
    	
    	$_POST['uid'] = $user->uid;
    	$_POST['name'] = $user->name;
    	$_POST['mail'] = $user->mail;
    	
    	if (!$error = module_invoke_all('user', 'newpass', (object)$_POST)) {
    	  $salt = user_get_salt_pass($_POST['pass']);
        $ok = false;
        
	      if (!$_SESSION['oldpass'] || $_SESSION['oldpass'] < time() - 300) {
          if ($_POST['pass'] != $_POST['oldpass'] 
          && user_is_login_pass(array('uid' => $user->uid), $_POST['oldpass'])) {
	          $ok = db_exec('UPDATE {users} SET pass = ?, salt = ? WHERE uid = ?',
	          array($salt['pass'], $salt['salt'], $user->uid));
          }
	      } else {
	        $ok = db_exec('UPDATE {users} SET pass = ?, salt = ? WHERE uid = ?',
	        array($salt['pass'], $salt['salt'], $user->uid));
	        unset($_SESSION['oldpass']);
	      }
	      
	      if ($ok) {
	        setcookie(session_name(), session_id(), -1, '/', $cookie_domain);
	        dd_set_message(t('user', '密码修改成功'));
	        dd_js_print('window.location.href = "'.url('user/center/newpass').'"');
	        
	        exit();
	      } else {
	        $error[] = t('user', '旧密码不匹配');
	      }
	      
      }
    }
    
    dd_js_print('$(\'#ajax_description\').html("'.addslashes(implode('<br />', dd_array2_to('value', $error))).'").show(300);');
    exit();
  }
 
  $form['settings'] = array(
    '#ajax_validate' => true,
    '#ajax_submit' => array(
      'des' => true,
      'options' => array(
        'dataType' => 'script',
        'url' => url('user/center/newpass', array('query' => array('ajax' => 'submit'))),
      ),
    ),
  );

  if (!$_SESSION['oldpass'] || $_SESSION['oldpass'] < time() - 300) {
    $form['fields']['oldpass'] = array(
      '#type' => 'password',
      '#title' => t('user', '旧密码'),
      '#required' => true,
    );
  }
  $form['fields']['pass'] = array(
    '#type' => 'password',
    '#title' => t('user', '新密码'),
    '#required' => true,
    '#attributes' => array(
      'id' => 'password',
    ),
  );
  $form['fields']['pass2'] = array(
    '#type' => 'password',
    '#title' => t('user', '重复新密码'),
    '#required' => true,
    '#attributes' => array(
      'class' => "{equalTo:'#password'}"
    ),
  );
  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#weight' => 100,
  );
  return $form;
}

/**
 * 激活帐号，触发 hook_user_status($old, &$ac)
 */
function user_page_activation() {
  if ($_GET['validate'] && $_GET['uid']) {
    if ($uid = db_query('SELECT uid FROM {users} WHERE validate = ? AND uid = ?',
    array($_GET['validate'], $_GET['uid']), array('return' => 'column'))) {
      if (db_exec('UPDATE {users} SET status = 1, validate = ? WHERE uid = ?', array('', $uid))) {
        
        $user = user_load($uid);
        
        module_invoke_all('user_status', 0, $user);
        
        if (user_login($user) && ($_GET['ajax'] != 'submit')) {
          dd_set_message(t('user', '激活成功，欢迎加入 !name', array('!name' => $GLOBALS['conf']['site_name'])));
          dd_goto(dd_get_token(var_get_key('user', 'redirect', 'user/center'), $user, array('user', 'system')));
        }
      }
    } else {
      dd_set_message(t('user', '不正确的激活链接'), 'error');
    }
  }
  
  $form['settings'] = array(
    '#validate' => array('user_activation_validate'),
  );

  $form['fields']['mail'] = array(
    '#type' => 'textfield',
    '#title' => t('user', '邮箱'),
    '#required' => true,
    '#description' => t('user', '请输入你注册时填写的邮箱，系统将为你重新发送激活链接'),
  );

  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('user', '发送激活链接')
  );

  return $form;
}

function user_activation_validate($form, $v) {
  if ($msg = form_vali_mail($v['mail'])) {
    $t['mail']['#error'][] = $msg;
  } else if (!$o = db_query('SELECT uid, name, created FROM {users} WHERE mail = ? AND status = 0',
  array($v['mail']), array('return' => 'one'))) {
    $t['mail']['#error'][] = t('user', '不正确的邮箱');
  } else {
    _user_activation($o->uid, $o->name, $v['mail'], $o->created);
    dd_set_message(t('user', '激活链接已发送，请注意查收'));
    dd_goto('user/activation');
  }
  return $t;
}

/**
 * 修改资料
 */
function user_page_edit() {
  
  return dd_get_form('user_edit_form', $GLOBALS['user']);
}

/**
 * 注册
 */
function user_page_register() {
  if ($_GET['ajax'] == 'submit') {
    $form_values = array();
    if (!$error = dd_ajax_validate(0, 'user_register_form', $_POST['form_token'])) {
      if ($account = user_insert($_POST['_dida_values'])) {
        if ($_GET['redirect'] && strpos($_GET['redirect'], 'user/login') === false && strpos($_GET['redirect'], 'user/register') === false) {
          $rediect = $_GET['redirect'];
        } else {
          $rediect = url(dd_get_token(var_get_key('user', 'redirect', 'user/center'), $account, array('user', 'system')));
        }
        if ($account->status == 1) {
          user_login(user_load($account->uid, true));
          dd_set_message(t('user', '恭喜，注册成功了'));
        } else if ($GLOBALS['conf']['user']['status'] == 0) {
          dd_set_message(t('user', '恭喜，注册成功了，你需要通过邮箱激活帐号'));
          $rediect = url('user/activation');
        } else if ($GLOBALS['conf']['user']['status'] == -1) {
          dd_set_message(t('user', '恭喜，注册成功了，请等待管理员激你的帐号'));
          $rediect = $GLOBALS['base_path'];
        }
        
        dd_js_print('window.location.href = "'.urldecode($rediect).'"');
      } else {
        dd_js_print('alert(\''.t('user', '抱歉，出现问题，注册失败了').'\')');
      }
    } else {
      dd_js_print('$(\'#ajax_description\').html("'. addslashes(implode('<br>', dd_array2_to('value', $error))).'").show(300);');
    }
  }

  $output = dd_get_form('user_register_form');

  return $output;
}

/**
 * 登录
 */
function user_page_login() {
  if ($_GET['ajax'] == 'submit') {
    if (!$error = dd_ajax_validate(0, 'user_login_form', $_POST['form_token'])) {
      if ($_SESSION['user']->uid > 0) {
        if (!$_POST['his'][1]) {
          global $cookie_domain;
          setcookie(session_name(), session_id(), -1, '/', $cookie_domain);
        }
        if ($_GET['redirect'] && strpos($_GET['redirect'], 'user/login') === false && strpos($_GET['redirect'], 'user/register') === false) {
          $rediect = $_GET['redirect'];
        } else {
          $rediect = url(dd_get_token(var_get_key('user', 'redirect', 'user/center'), $_SESSION['user'], array('user', 'system')));
        }
        dd_js_print('location.href = "'.urldecode($rediect).'"');
      } else {
        dd_js_print('alert(\''.t('user', '抱歉，登录失败了').'\')');
      }
    } else {
      dd_js_print('$(\'#ajax_description\').html("'.addslashes(implode('<br>', dd_array2_to('value', $error))).'").show(300);');
    }
  }
  $v = dd_get_form('user_login_form');
  return $v;
}

function user_page_login_ajax() {
  if ($_GET['ajax'] == 'submit') {
    if (!$error = dd_ajax_validate(0, 'user_login_form', $_POST['form_token'])) {
      if ($_SESSION['user']->uid > 0) {
        if (!$_POST['his'][1]) {
          global $cookie_domain;
          setcookie(session_name(), session_id(), -1, '/', $cookie_domain);
        }
        switch ($_GET['call']) {
          case 'to':
            //关闭本页，刷新父页
            dd_js_print('self.parent.dialog_colse()');
          break;
          case 'function':
            //执行指定函数
            $_GET['function'] ? dd_js_print('self.parent.dialog_colse(); self.parent.'. $_GET['function'] .'();') : dd_js_print('self.parent.dialog_colse();');
          break;
          case 'close':
            //关闭层
            dd_js_print('self.parent.dialog_colse();');
          break;
          default:
            $url = $_GET['redirect'] ? $_GET['redirect'] : url(dd_get_token(var_get_key('user', 'redirect', 'user/center'), $_SESSION['user'], array('user', 'system')));
            dd_js_print('window.parent.location.href = "'.urldecode($url).'"');
        }
      } else {
        dd_js_print('alert(\''.t('user', '抱歉，登录失败了').'\')');
      }
    } else {
      dd_js_print('$(\'#ajax_description\').html("'.addslashes(implode('<br>', dd_array2_to('value', $error))).'").show(100);');
    }
    exit;
  }
  echo theme('print', dd_get_form('user_login_form'));
  exit;
}
