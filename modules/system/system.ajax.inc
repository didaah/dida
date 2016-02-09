<?php
// $Id$

/**
 * 全局 ajax 地址，方便模块创建 ajax，省去定义 hook_menu 的步骤
 * 不验证权限，请在 module_system_tools_ajax 中自行验证
 */
function system_tools_ajax() {
  $module = !empty($_POST['module']) ? $_POST['module'] : $_GET['module'];
  
  if ($module && isset($GLOBALS['conf']['modules'][$module])) {
    module_include_file('system.ajax.inc', $module);
    // 传递 $_POST、$_GET
    $type = !empty($_POST['op']) ? $_POST['op'] : $_GET['op'];
    echo module_invoke($module, 'system_tools_ajax', $_POST, $_GET, $type);
  }
  
  exit;
}

/**
 * @Implement of hook_system_tools_ajax()
 */
function system_system_tools_ajax($v, $g, $type) {
  switch ($type) {
    case 'admin_menu':
      if (is_array($v['system_admin_menu_list'])) {
        if (!user_access('system view admin')) return false;
        
        global $user;

        $table = array_flip($v['system_admin_menu_list']);
        cache_set('_admin_menu_list_'.$user->uid, $table);
        
        $language = $user->language ? $user->language : $GLOBALS['conf']['default_language'];
        
        if ($data = cache_get('admin_menus_' . $user->uid .'_'. $language)) {
          if (is_array($data->data)) {
            foreach ($data->data as $module => $menu) {
              $data->data[$module]['#weight'] = $table[$module];
            }
            uasort($data->data, 'dd_form_cmp');
            cache_set('admin_menus_' . $user->uid .'_'. $language, $data->data);
          }
        }
        
        return true;
      }
    break;
    case 'block_set_link':
      $json = array('path' => '');
      
      if ($_POST['path'] && $_POST['title']) {
        if ($v = system_set_custom_block_links($_POST)) {
          $json = $v->data;
          $json['path'] = url($v->data['path']);
        }
      }
    return json_encode($json);
    
    case 'block_delete_link':
      if ($_POST['id']) {
        $return = system_del_custom_block_links($_POST['id']);
      }
    return $return;

    case 'lang_change':
      if (user_access('system lang change')) {
        global $conf, $user;
        
        if (!empty($conf['languages']) && !empty($g['lang']) && !empty($conf['languages'][$g['lang']])) {
          
          $_SESSION['user']->language = $g['lang'];

          if ($user->uid > 0) {
            db_exec('UPDATE {users} SET language = ? WHERE uid = ?', array($g['lang'], $user->uid));
            user_clear_session($user->uid);
            dd_set_cookie('lang', $g['lang']);
          }
        } else {
          dd_set_message(t('system', '沒有语言供选择'));
        }

        dd_goto_redirect();
      }
    exit;
  }
}
