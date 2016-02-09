<?php
// $Id$

// public system_modules_page() {{{ 
/**
 * system_modules_page
 *  模块管理
 * 
 * @access public
 * @return void
 */
function system_modules_page() {
  switch (arg(2)) {
    case 'update':
      global $conf;
      if ($_GET['name'] && $conf['modules'][$_GET['name']]) {
        $modules = array($_GET['name']);
      } else {
        $modules = array_keys($conf['modules']);
      }
      
      $query = module_set_enabled('enabled', 'module', $modules);
      
      if (!empty($query) && is_array($query)) {
        dd_set_message('升级成功，升级中执行了以下 sql：<br>' .implode('<br>', $query));
      } else {
        dd_set_message(t('system', '没有模块需要升级'));
      }
      
      dd_goto('admin/modules');
    break;
    case 'uninstall':
      if ($_GET['name'] && ($module = module_set_uninstall($_GET['name']))) {
        dd_set_message(t('system', '%string 成功卸载', array('%string' => $module)));
      }
      dd_goto('admin/modules');
  }
  
  dd_set_tabs(array(
    l(t('system', '模块列表'), 'admin/modules'),
    l(t('system', '模块升级'), 'admin/modules/update'),
    l(t('system', '主题管理'), 'admin/themes')
  ));
  
  $modules = module_list('module');
  
  foreach ($modules as $name => $module) {
    /*
    $schema = $data = array();
    $filepath = $module['path']. '/'. $name . '.install';
    if (is_file($filepath)) {
      if ($fp = fopen($filepath, 'rb')) {
        while (!feof($fp)) {
          $line = trim(fgets($fp));
          if (substr($line, 0, 8) == '$schema[') {
            $schema[] = $line;
          }
        }
        fclose($fp);
        if ($schema) {
          foreach ($schema as $val) {
            if (preg_match('/schema\[\'(.*?)\'\]/i', $val, $mat)) {
              $data[$mat[1]] = $mat[1];
            }
          }
        }
      }
    }
    */
    $form['fields']['modules'][$module['filename']]['status']['#value'] = $module['status'];
    $form['fields']['modules'][$module['filename']]['name']['#value'] = $module['info']['name'] ? $module['info']['name'] : $name;
    $form['fields']['modules'][$module['filename']]['path']['#value'] = $module['path'];
    $form['fields']['modules'][$module['filename']]['filename']['#value'] = $module['filename'];
    
    $form['fields']['modules'][$module['filename']]['ver']['#value'] = $module['info']['ver'];
    $form['fields']['modules'][$module['filename']]['install']['#value'] = $module['install'];
    $form['fields']['modules'][$module['filename']]['des']['#value'] = $module['info']['description'];
    $form['fields']['modules'][$module['filename']]['check'] = array(
      '#type' => 'checkbox',
      '#default_value' => $module['status'],
      '#attributes' => array(
        'title' => t('system', '启用这个模块'),
      ),
    );
    
    if ($module['info']['dependencies']) {
      foreach ($module['info']['dependencies'] as $key => $dependencies) {
        if (!$modules[$dependencies]) {
          $form['fields']['modules'][$module['filename']]['check']['#disabled'] = true;
          $module['info']['dependencies'][$key] = $dependencies.'<span class="red">'.t('system', '(缺少)').'</span>';
        } else {
          $module['info']['dependencies'][$key] = t($dependencies, $modules[$dependencies]['info']['name']).'('.$dependencies.')';
        }
      }
      $form['fields']['modules'][$module['filename']]['dependencies']['#value'] = $module['info']['dependencies'];
    }
    
    if (in_array($name, module_core_module())) {
      $form['fields']['modules'][$module['filename']]['check']['#disabled'] = true;
      $form['fields']['modules'][$module['filename']]['check']['#default_value'] = 1;
    } else if ($module['status'] && $module['info']['uninstall']) {
      $form['fields']['modules'][$module['filename']]['uninstall']['#value'] = true;
    }
  }
  
  $form['settings'] = array(
    '#validate' => array('system_modules_validate'),
    '#theme' => 'system_modules_theme'
  );

  $form['fields']['submit']['#type'] = 'submit';
  
  return $form;
}
// }}}

function system_modules_validate($form, $value) {
  global $conf;

  // 已启用模块列表
  $list = module_list('module', 'enabled');

  foreach ($form['fields']['modules'] as $name => $info) {
    if (!empty($value['modules'][$name]['check'])) {
      if (empty($list[$name])) $enabled[] = $name;
    } else if (!empty($conf['modules'][$name])) {
      // 核心模块不允许禁用
      if (!in_array($name, module_core_module())) $disabled[] = $name;
    }
  }
  
  if ($enabled) {
    $query = module_set_enabled('all', 'module', $enabled);
    if ($query !== false) {
      dd_set_message(t('system', '%string 启用成功', array('%string' => implode(',', $enabled))));
      if (count($query)) {
        dd_set_message(implode('<br />', $query));
      }
    }
  }
  
  if ($disabled && module_set_disabled($disabled)) {
    dd_set_message(t('system', '%string 已禁用', array('%string' => implode(',', $disabled))));
  }
  
  if ($enabled || $disabled) {
    cache_del('cid', 'cache_hook');
    theme_get_cache();
  }
}

function system_modules_theme($form, $fields) {
  $header = array(
    array('data' => t('system', '启用'), 'class' => 'checkbox'),
    array('data' => t('system', '模块信息'), 'class' => 'info'),
    array('data' => t('system', '说明'), 'class' => 'description'),
    array('data' => t('system', '操作'), 'class' => 'op')
  );

  $rows = $table = array();

  foreach ($fields['modules'] as $module) {
    $item = array(
      '<strong>'.t('system', '名称：').'</strong>' . $module['name']['#value'],
      '<strong>'.t('system', '版本：').'</strong>' . $module['ver']['#html'],
      '<strong>'.t('system', '路径：').'</strong>' . $module['path']['#value'],
    );
    if ($module['dependencies']['#value']) {
      $item[] = '<strong>'.t('system', '依赖：').'</strong>'. implode(', ', $module['dependencies']['#value']);
    }
    $v = module_get_ver($module['filename']['#value'], 1);
    
    $links = array();

    if ($module['status']['#value'] == 1) {
      if (($module['install']['#value'] == -1 && $v['install']) 
      || ($module['install']['#value'] != -1 && $module['install']['#value'] < $v['ver'])) {
        $links[] = l(t('system', '升级'), 'admin/modules/update', array('query' => 'name='.$module['filename']['#value']));
      }
      if ($module['uninstall']['#value']) {
        $links[] = l(t('system', '卸载'), 'admin/modules/uninstall', array('query' => 'name='.$module['filename']['#value'], 'attributes' => array('class' => 'confirm')));
      }
    }
    
    $row = array($module['check']['#html'], theme('item_list', $item), $module['des']['#value'], implode(' | ', $links));
    
    if ($module['status']['#value'] == 1) {
      $table[] = $row;
    } else {
      $rows[] = $row;
    }
  }
  
  if ($rows) {
    $table = array_merge($table, $rows);
  }
  
  $table[] = array(array(
    'data' => $fields['submit']['#html'],
    'colspan' => 6,
    'class' => 'table_submit'
  ));
  return theme('table', $header, $table);
}

/**
 * 管理界面专用主题
 */
function system_themes_admin() {
  dd_set_tabs(array(
    l(t('system', '模块管理'), 'admin/modules'),
    l(t('system', '主题管理'), 'admin/themes'),
    l(t('system', '管理专用主题'), 'admin/themes/admin'),
  ));
  
  $options[0] = t('system', '默认');
  foreach ($GLOBALS['conf']['themes'] as $name => $theme) {
    $options[$name] = $theme['name'];
  }
  
  $form['fields']['theme_admin'] = array(
    '#type' => 'select',
    '#title' => t('system', '管理页专用主题'),
    '#default_value' => var_get('theme_admin', 0),
    '#options' => $options
  );
  return system_setting_form($form);
}

/**
 * 主题管理
 */
function system_themes_page() {
  global $conf;
  
  dd_set_tabs(array(
    l(t('system', '模块管理'), 'admin/modules'),
    l(t('system', '主题管理'), 'admin/themes'),
    l(t('system', '管理专用主题'), 'admin/themes/admin'),
  ));
  
  if ($_GET['op'] == 'default' && $_GET['name']) {
    if (db_exec('UPDATE {system} SET status = -1 WHERE filename = ?', array($_GET['name']))) {
      db_exec('UPDATE {system} SET status = 1 WHERE status = -1 AND filename != ?', array($_GET['name']));
      cache_del('*', 'admin_menus_');
      var_set('default_theme',  $_GET['name']);
      dd_set_message(t('system', '%string 已被设为默认主题，请设置区块', array('%string' => $_GET['name'])));
      dd_goto('admin/blocks/settings/'.$_GET['name']);
    }
    dd_goto($_GET['q']);
  }
  
  foreach (module_list('theme') as $name => $theme) {
    $filename = $theme['info']['name'] ? $theme['info']['name'] : $name;
    
    $form['fields']['themes'][$theme['filename']]['check'] = array(
      '#type' => 'checkbox',
      '#default_value' => $theme['status'],
      '#attributes' => array(
        'title' => t('system', '启用'),
      ),
    );
    if ($theme['status'] == -1) {
      $form['fields']['themes'][$theme['filename']]['check'] = array(
        '#type' => 'checkbox',
        '#default_value' => $theme['status'],
        '#description' => t('system', '当前默认'),
        '#disabled' => 1,
        '#attributes' => array(
          'title' => t('system', '当前默认'),
        ),
      );
    } else if ($theme['status'] == 1) {
      $form['fields']['themes'][$theme['filename']]['check'] = array(
        '#type' => 'checkbox',
        '#default_value' => $theme['status'],
        '#description' => l(t('system', '设为默认'), $_GET['q'], array('query' => 'op=default&name='.$theme['filename'])),
        '#attributes' => array(
          'title' => t('system', '禁用'),
        ),
      );
    } else {
      $form['fields']['themes'][$theme['filename']]['check'] = array(
        '#type' => 'checkbox',
        '#default_value' => $theme['status'],
        '#attributes' => array(
          'title' => t('system', '启用'),
        ),
      );
    }
    
    $form['fields']['themes'][$theme['filename']]['filename']['#value'] = $theme['filename'];
    $form['fields']['themes'][$theme['filename']]['name']['#value'] = $filename;
    $form['fields']['themes'][$theme['filename']]['path']['#value'] = $theme['path'];
    $form['fields']['themes'][$theme['filename']]['ver']['#value'] = $theme['info']['ver'];
    $form['fields']['themes'][$theme['filename']]['des']['#value'] = $theme['info']['description'];
    if ($theme['info']['preview_thumb']) {
      $form['fields']['themes'][$theme['filename']]['thumb']['#value'] = l(
        img($theme['path'].'/'.$theme['info']['preview_thumb'], t('system', '预览')),
        f($theme['path'].'/'.$theme['info']['preview_original']),
        array('html' => 1, 'attributes' => array('target' => '_blank', 'title' => t('system', '查看大图')))
      );
    } else {
      $form['fields']['themes'][$theme['filename']]['thumb']['#value'] = t('system', '无');
    }
  }
  
  $form['settings'] = array(
    '#validate' => array('system_themes_validate'),
    '#theme' => 'system_themes_theme'
  );

  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('system', '确认操作'),
  );

  return $form;
}

function system_themes_theme($form, $fields) {
  if (!empty($_GET['name']) && $_GET['op'] == 'install') {
    // 启用主题
    if (db_exec('UPDATE {system} SET status = 1 WHERE filename = ? AND type = ?',
    array($_GET['name'], 'theme'))) {
      dd_set_message(t('system', '启用成功'));
      module_enabled_variable('theme');
      dd_goto($_GET['q']);
    }
  }

  $header = array(
    t('system', '预览'),
    t('system', '名称'),
    t('system', '启用'),
    array(
      'data' => t('system', '路径与说明'),
      'class' => 'path'
    ),
    t('system', '版本'),
    t('system', '操作'),
  );
  
  foreach ($fields['themes'] as $theme) {
    $table[] = array(
      $theme['thumb']['#value'],
      $theme['name']['#value'],
      $theme['check']['#html'],
      $theme['path']['#value'] . '<br/>' . $theme['des']['#value'],
      $theme['ver']['#value'],
      (!empty($theme['check']['#value']) ? l(t('system', '设置'), 'admin/themes/setting/' . $theme['filename']['#value']) 
      : l(t('system', '启用'), $_GET['q'], array('query' => 'op=install&name=' . $theme['filename']['#value'])))
    );
  }

  $table[] = array(array(
    'data' => $fields['submit']['#html'],
    'colspan' => 7,
    'class' => 'table_submit',
  ));
  return theme('table', $header, $table, array('class' => 'admin_themes_table'));
}

function system_themes_validate($form, $value) {
  foreach ($form['fields']['themes'] as $name => $info) {
    if ($name == $GLOBALS['conf']['default_theme']) {
      continue;
    }
    if ($value['themes'][$name]['check']) {
      if (db_exec('UPDATE {system} SET status = 1 WHERE filename = ?', array($name))) {
        dd_set_message(t('system', '%string 已启用', array('%string' => $name)));
      }
    } else {
      if (db_exec('UPDATE {system} SET status = 0 WHERE filename = ?', array($name))) {
        dd_set_message(t('system', '%string 已禁用', array('%string' => $name)));
      }
    }
  }
  module_enabled_variable('theme');
}

function system_themes_setting($theme) {
  global $conf;
  
  if (!$conf['themes'][$theme]) {
    dd_get_not();
  }
  
  dd_set_tabs(array(
    l(t('system', '模块管理'), 'admin/modules'),
    l(t('system', '主题管理'), 'admin/themes'),
    l(t('system', '主题设置'), $_GET['q']),
  ));
  
  $form = array('fields' => array(), 'settings' => array());

  $form['fields']['title'] = array(
    '#title' => t('system', '主题名称'),
    '#value' => $conf['themes'][$theme]['name']
  );

  $form['fields']['theme'] = array(
    '#type' => 'hidden',
    '#constant' => 1,
    '#default_value' => $theme,
  );

  $form['fields']['logo'] = array(
    '#type' => 'file',
    '#title' => t('system', '站点LOGO'),
    '#description' => t('system', '请上传尺寸适合的图片，系统将不做任何处理'),
  );

  $form['fields']['favicon'] = array(
    '#type' => 'file',
    '#title' => t('system', 'icon图标'),
    '#description' => t('system', '请上传尺寸适合的图片，系统将不做任何处理'),
  );

  $options = array();
  foreach (glob(DIDA_ROOT . '/misc/ui/themes/*') as $path) {
    if (is_dir($path) && is_file($path . '/jquery-ui.min.css')) {
      $dir = basename($path);
      $options[$dir] = $dir;
    }
  }

  $form['fields']['ui_theme'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => !empty($conf['themes'][$theme]['ui_theme']) ? $conf['themes'][$theme]['ui_theme'] : 'base',
    '#title' => t('system', 'UI Theme'),
    '#description' => t('system', '可指定网站全局所使用的 jQuery UI Theme'),
  );
   
  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#weight' => 1000
  );

  $form['settings']['#validate'][] = 'system_themes_setting_validate';

  if (is_file($conf['themes'][$theme]['path'].'/template.php')) {
    
    if (!isset($conf['themes'][$theme]['settings'])) {
      $conf['themes'][$theme]['settings'] = array();
    }
    
    require_once $conf['themes'][$theme]['path'] . '/template.php';
    $function = $theme.'_settings_form';
    if (function_exists($function)) {
      $function($form, $conf['themes'][$theme]);
    }
  }
  
  return $form;
}

function system_themes_setting_validate(&$form, &$v) {
  if ($theme = db_query('SELECT filename, info FROM {system} WHERE filename = ? AND type = ?',
  array($v['theme'], 'theme'), array('return' => 'one'))) {
    $theme->info = unserialize($theme->info);
    
    if (!$error = file_validate_error($_FILES['logo']['error'])) {
      if ($file = file_save_upload($_FILES['logo'],
        array('image' => true, 'savepath' => 'cache/themes', 'no_insert' => 1, 'filename' => $v['theme'].'_logo'), 0)) {
        
        if (!empty($theme->info['logo']) && $theme->info['logo'] != $file->filepath) {
          @unlink($theme->info['logo']);
        }
        $theme->info['logo'] = $file->filepath;
      }
    }
    
    if (!$error = file_validate_error($_FILES['favicon']['error'])) {
      if ($file = file_save_upload($_FILES['favicon'],
        array('image' => true, 'savepath' => 'cache/themes', 'no_insert' => 1, 'filename' => $v['theme'].'_favicon'), 0)) {
        if (!empty($theme->info['favicon']) && $theme->info['favicon'] != $file->filepath) {
          @unlink($theme->info['favicon']);
        }
        $theme->info['favicon'] = $file->filepath;
      }
    }
    
    $theme->info['settings'] = $v['settings'];
    db_exec('UPDATE {system} SET info = ? WHERE filename = ?', array(serialize($theme->info), $theme->filename));
    
    module_enabled_variable('theme');
    var_init();
    
    dd_set_message(t('system', '操作成功'));
    
  }
}
