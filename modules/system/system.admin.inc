<?php
// $Id$

/**
 * 管理首页区块，提供钩子 hook_system_admin_default()
 * 模块可自定义一个区块在管理首页显示
 */
function system_admin_default() {
  global $conf;
  
  if ($_GET['op'] == 'system' && $_GET['view'] == 'php') {
    phpinfo();
    exit();
  }
  
  $output = '<h1>'.t('system', '欢迎进入 %string 管理系统', array('%string' => $conf['site_global']['name'])).'</h1>';

  if ($data = _module_invoke_all('system_admin_default')) {
    $i = 0;
    foreach ($data as $module => $content) {
      $html = '<div class="system_admin_default_block system_admin_default_block_'.$module.'">';
      if (is_array($content)) {
        if ($content['#title']) {
          $html .= '<h2 class="title">'.$content['#title'].'</h2>';
        }
        if ($content['#content']) {
          $html .= $content['#content'];
          if ($content['#weight']) {
            $weight = $content['#weight'];
          } else {
            $weight = $i;
          }
        }
      } else {
        $html .= $content;
        $weight = $i;
      }
      $html .= '</div>';
      $lists[] = array(
        '#content' => $html,
        '#weight' => $weight,
      );
      ++$i;
    }
    uasort($lists, 'dd_form_cmp');
    foreach ($lists as $d) {
      $output .= $d['#content'];
    }
  }
  return $output;
}

// public system_admin_site_menu() {{{ 
/**
 * system_admin_site_menu
 *  站点主导航设置
 * 
 * @access public
 * @return string
 */
function system_admin_site_menu() {
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '主导航设置')));

  dd_set_tabs(array(
    l(t('system', '自定义导航'), 'admin/site_menu'),
    l(t('system', '新增导航'), 'admin/site_menu/add', array('query' => 'field_field_type=select')),
    l(t('system', '系统导航'), 'admin/site_menu/system'),
  ));

  switch (arg(2)) {
    case 'system':
      $menus = array();

      module_alter_all('site_menu', $menus);

      $item = array();

      foreach ($menus as $key => $menu) {
        if (is_array($menu)) {
          $rows = $menu['#data'] . '-----' . t('system', '系统名称：%name', array('%name' => $key));
          if (!empty($menu['#childrens'])) {
            $_item = array();
            foreach ($menu['#childrens'] as $data) {
              if (is_array($data)) {
                $_item[] = $data['#data'];
              } else {
                $_item[] = $data;
              }
            }
            $rows .= theme('item_list', $_item);
          }
        } else {
          $rows = $menu . '-----' . t('system', '系统名称：!name', array('!name' => $key));
        }
        $item[] = $rows;
      }
      dd_set_help(t('system', '系统导航由模块生成，只可以禁用，无法删除。以下显示的并非全部，某些导航因权限判断，你可能无法看到。'));
      $output = dd_get_form('_system_admin_site_menu_system');
      $output .= theme('item_list', $item, '系统导航', 'ul', array('class' => 'admin_site_menu'));
    break;
  	case 'add':
      if ($_GET['field_field_type'] != 'select') {
        dd_goto('admin/site_menu/add', 'field_field_type=select');
      }

      $field->type = 'menu';
      $field->module = 'system';
      $field->redirect = url('admin/site_menu');
  		$output = dd_get_form('field_form', $field);
  	break;
    default:
      // 不允许批量添加
      if (!empty($_GET['field_custom']) && $_GET['field_custom'] == 'import') {
        dd_set_message(t('system', '不允许此操作'));
        dd_goto('admin/site_menu');
      }
      $output = field_lists('system', 'menu', array('name', 'links'));
  }

  return $output;
}
// }}}

/**
 * 系统主导航设置表单
 */
function _system_admin_site_menu_system() {
  $form = array(
    'fields' => array(),
    'settings' => array('#validate' => array('_system_admin_site_menu_system_validate'))
  );

  $disabled = $weight = '';

  if ($data = var_get('site_menu_system')) {
    if (!empty($data['disabled'])) {
      $disabled = implode(',', array_flip($data['disabled']));
    }

    if (!empty($data['weight'])) {
      foreach ($data['weight'] as $key => $value) {
        $weight .= ',' . $key . ':' . $value;
      }
      $weight = substr($weight, 1);
    }
  }

  $form['fields']['disabled'] = array(
    '#title' => t('system', '禁用导航'),
    '#default_value' => $disabled,
    '#type' => 'textfield',
    '#description' => t('system', '输入需禁用的导航系统名称，多个以逗号分隔，如：user,front')
  );

  $form['fields']['weight'] = array(
    '#title' => t('system', '导航权重'),
    '#default_value' => $weight,
    '#type' => 'textfield',
    '#description' => t('system', '自定义导航权重，多个以逗号分隔，如：user:1000,front:1001')
  );

  $form['fields']['button'] = array('#type' => 'submit');

  return $form;
}

function _system_admin_site_menu_system_validate(&$form, &$v) {
  $weight = array();
  
  if (!empty($v['weight'])) {
    if ($array = field_get_string_tags($v['weight'])) {
      foreach ($array as $value) {
        if (strpos($value, ':') !== false) {
          list($name, $id) = explode(':', $value, 2);
          $weight[$name] = $id;
        }
      }
    }
  }
  
  if ($array = field_get_string_tags($v['disabled'])) {
    $disabled = array_flip($array);
  } else {
    $disabled = '';
  }

  $data = array(
    'disabled' => $disabled,
    'weight' => $weight,
  );

  var_set('site_menu_system', $data);

  dd_set_message(t('system', '操作成功'));
}

/**
 * @Implement of hook_system_admin_default()
 */
function system_system_admin_default() {
  global $conf;
  $item[] = dd_get_strong(t('system', '网站名称')). $conf['site_global']['name'] . l(t('system', '修改'), 'admin/settings/global');
  
  $item[] .= dd_get_strong(t('system', '程序版本')). DIDA_VERSION.l(t('system', '检查更新'), 'admin/status/update', dd_redirect());
  
  $item[] .= dd_get_strong(t('system', '站长邮箱')). $conf['site_global']['mail'] . l(t('system', '修改'), 'admin/settings/global');
  $item[] .= dd_get_strong(t('system', '创建时间')). format_date($conf['site_created']);
  
  if ($conf['cron_last_time']) {
    $cron = format_date($conf['cron_last_time']);
  } else {
    $cron = t('system', '从未执行');
  }
  if ($conf['cron_pass']) {
    $cron_url = f('cron.php?pass='.md5($conf['cron_pass']));
  } else {
    $cron_url = f('cron.php');
  }
  
  $cron .= l(t('system', '立即执行'), $cron_url, array('attributes' => array('target' => '_blank')));
  
  $item[] .= dd_get_strong(t('system', '上次 CRON')) . $cron;
  
  $item[] .= dd_get_strong(t('system', 'PHP 版本')) . PHP_VERSION. l(t('system', '查看'), 'admin/status/php');
  
  $item[] .= dd_get_strong(t('system', '数据库')) . $GLOBALS['database']['default']['driver'] .' - '. db_get_version();
  $item[] .= dd_get_strong(t('system', '服务器')). PHP_OS .' - ' .$_SERVER['SERVER_SOFTWARE'];
  
  $content = array(
    '#title' => t('system', '系统信息'),
    '#content' => theme('item_list', $item)
  );
  
  return $content;
}

/**
 * robots.txt 设置
 * 当一套代码多个站点时，使用物理文件 robots.txt，将导致不能自定义
 * 所以提供管理后台添加，动态生成 robots.txt 的功能
 */
function system_admin_robots() {
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', 'robots.txt 设置')));

  $form = array('fields' => array(), 'settings' => array('#validate' => array('system_admin_robots_validate')));
  
$data = <<<EOT
User-agent: *
Crawl-delay: 10

Disallow: /includes/
Disallow: /misc/
Disallow: /modules/
Disallow: /sites/logs/
Disallow: /sites/cache/
Disallow: /sites/themes/
Disallow: /sites/modules/
Disallow: /themes/
Disallow: /install/
Disallow: /tools/

Disallow: /rpc.php
Disallow: /cron.php
Disallow: /custom.php
Disallow: /install.php

Disallow: /admin
Disallow: /ajax
Disallow: /captcha
Disallow: /g
Disallow: /search
Disallow: /images/error
Disallow: /user/logout
Disallow: /user/register
Disallow: /user/pass
Disallow: /user/login

# 若不启用简洁链接，删除下一行
Disallow: /?q
EOT;

  if ($cache = custom_get('system_robots')) {
    $data = $cache;
  }

  $form['fields']['data'] = array(
    '#title' => t('system', 'robots.txt'),
    '#default_value' => $data,
    '#type' => 'textarea',
    '#description' => t('system', '自定义 robots.txt 文件内容')
  );

  $form['fields']['button'] = array(
    '#type' => 'submit'
  );

  return $form;
}

function system_admin_robots_validate(&$form, &$v) {
  custom_set('system_robots', $v['data']);
  dd_set_message(t('system', '操作成功'));
}

/**
 * phpinfo
 */
function system_status_php() {
  phpinfo();
  exit;
}

/**
 * 系统日志、访问记录
 */
function system_admin_logs(){
  $header = array(
    array('data' => t('system', '用户'), 'field' => 'l.uid'),
    array('data' => t('system', '时间'), 'field' => 'l.sid', 'sort' => 'desc'),
    array('data' => t('system', '类型'), 'field' => 'l.type'),
    array('data' => t('system', '状态'), 'field' => 'l.status'),
    array('data' => t('system', '主机'), 'field' => 'l.host'),
    array('data' => t('system', '路径'), 'field' => 'l.url'),
  );
  
  $breadcrumb = array(l(t('system', '系统管理'), 'admin'));

  $types = module_invoke_all('logs_info');
  $status = dd_logs_get_status();

  $query = 'SELECT l.*, u.name FROM {logs} l LEFT JOIN {users} u ON l.uid = u.uid';
  $args = NULL;

  if (arg(2) && !empty($types[arg(2)])) {
    $query .= ' WHERE l.type = ?';
    $args = array(arg(2));
    if (!empty($_GET['keywords'])) {
      $query .= ' AND l.value LIKE ?';
      $args[] = '%' . $_GET['keywords'] . '%';
    }
    $breadcrumb[] = l(t('system', '系统日志'), 'admin/logs');
    $breadcrumb[] = $types[arg(2)];
  } else {
    if (!empty($_GET['keywords'])) {
      $query .= ' WHERE l.value LIKE ?';
      $args = array('%' . $_GET['keywords'] . '%');
    }
    $breadcrumb[] = t('system', '系统日志');
  }

  if($fetch = pager_query($query . table_sql($header), 50, 0, NULL, $args)){
    foreach($fetch as $o){
      $table[] = array(
        theme('username', $o),
        format_date($o->timestamp),
        $types[$o->type] ? $types[$o->type] : $o->type,
        $status[$o->status],
        $o->host,
        $o->url,
      );
      $table[][] = array(
        'data' => dd_get_strong(t('system', '说明')).$o->value,
        'colspan' => 6
      );
    }

    $item = array(l(t('system', '所有日志'), 'admin/logs'));
    foreach ($types as $key => $name) {
      $item[] = l($name, 'admin/logs/' . $key);
    }

    $output = dd_get_form('system_admin_logs_delete_form');
    $output .= '<p>' . dd_get_strong(t('system', '按日志类型查看')) . dd_imp($item) . '</p>';
    $output .= theme('table', $header, $table);
    $output .= pager_view();
  } else {
    $output = system_no_content();
  }
  
  dd_set_breadcrumb($breadcrumb);

  return $output;
}

/**
 * 系统日志删除
 */
function system_admin_logs_delete_form() {
  $form['fields']['all'] = array(
    '#type' => 'checkbox',
    '#title' => t('system', '清空全部日志'),
  );

  if ($GLOBALS['_filter_is_php_code'])  {
    $form['fields']['query'] = array(
      '#type' => 'textfield',
      '#title' => t('system', '或自定义删除条件'),
      '#field_prefix' => 'DELETE FROM `logs` WHERE ',
      '#description' => t('system', '请补充 sql语句，如：sid < 1000。注意，错误的 SQL 将有未知风险')
    );
  }

  $form['fields']['keywords'] = array(
    '#type' => 'textfield',
    '#default_value' => empty($_GET['keywords']) ? urldecode($_GET['keywords']) : '',
    '#title' => t('system', '搜索日志'),
  );
 
  $form['fields']['botton']['#type'] = 'submit';
  
  $form['settings'] = array(
    '#validate' => array('system_admin_logs_delete_form_validate') 
  );
  
  return $form; 
}

function system_admin_logs_delete_form_validate($form, $v) {
  
  if ($v['all']) {
    db_exec('TRUNCATE TABLE {logs}');
  } else if ($v['query'] && $GLOBALS['_filter_is_php_code']) {
    db_exec('DELETE FROM {logs} WHERE ' . $v['query']);
  } else if (!empty($v['keywords'])) {
    dd_goto($_GET['q'], 'keywords=' . urlencode($v['keywords'])); 
  }
  
  dd_set_message(t('system', '操作成功'));
}

/**
 * 文件设置
 */
function system_site_files() {
  $form['fields']['file_upload_rename'] = array(
    '#type' => 'radio',
    '#required' => 1,
    '#title' => t('system', '重命名'),
    '#description' => t('system', '将上传文件以 md5 方式重命名，可兼容中文名称上传'),
    '#options' => array(t('system', '否'), t('system', '是')),
    '#default_value' => var_get('file_upload_rename', 0)
  );
  $form['fields']['file_directory_path'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#title' => t('system', '根目录'),
    '#description' => t('system', '相对于程序目录的系统路径，所有文件将存储于该文件夹'),
    '#default_value' => var_get('file_directory_path', $GLOBALS['conf_dir'].'/files')
  );
  $form['fields']['file_user_directory'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#attributes' => array('size' => 30),
    '#title' => t('system', '用户目录'),
    '#description' => t('system', '相对于根目录，用户文件默认存储于此。可使用 user、system 通配符，<a href="!url">通配符列表</a>', array('!url' => url('admin/token'))),
    '#default_value' => var_get('file_user_directory', 'user-[uid]/[date_Y]-[date_m]')
  );
  $form['fields']['user_file_size'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#attributes' => array('size' => 10),
    '#title' => t('system', '用户默认容量'),
    '#field_suffix' => t('system', 'MB'),
    '#description' => t('system', '0 为不限制。也可按角色设置。容量限制仅在模块发出检查请求时生效，详见 file.inc'),
    '#default_value' => var_get('user_file_size', 30)
  );
  if ($roles = user_data_roles()) {
    $i = 0;
    foreach ($roles as $rid => $role) {
      if ($rid != var_get_key('user', 'anonymous_role', 1)) {
        $form['fields']['user_file_size_role_'.$rid] = array(
          '#type' => 'textfield',
          '#required' => 1,
          '#attributes' => array('size' => 10),
          '#title' => t('system', $role->name),
          '#field_suffix' => t('system', 'MB'),
          '#default_value' => var_get('user_file_size_role_'.$rid, 0)
        );
        if ($i == 0) {
          $form['fields']['user_file_size_role_'.$rid]['#fieldset_prefix'] = 'asc';
          $form['fields']['user_file_size_role_'.$rid]['#fieldset_legend'] = t('system', '按角色分配空间');
        }
        $i = $rid;
      }
    }
    $form['fields']['user_file_size_role_'.$i]['#fieldset_suffix'] = 1;
  }
   
  $form['fields']['file_image_quality'] = array(
    '#type' => 'textfield',
    '#default_value' => var_get('file_image_quality', 75),
    '#title' => t('system', '图片压缩质量'),
  );
  
  $form['fields']['file_image_watermark'] = array(
    '#fieldset_suffix' => 1,
    '#fieldset_prefix' => 'asc',
    '#fieldset_legend' => t('system', '图片水印设置'),
  );
  
  $form['fields']['file_image_watermark']['default'] = array(
    '#type' => 'radio',
    '#required' => 1,
    '#title' => t('system', '开启水印'),
    '#description' => t('system', '自动为所有图片添加水印'),
    '#options' => array(t('system', '否'), t('system', '是')),
    '#default_value' => var_get_key('file_image_watermark', 'default', 0)
  );
  
  $form['fields']['file_image_watermark']['Ufile'] = array(
    '#type' => 'file',
    '#title' => t('system', '水印图片'),
    '#description' => t('system', '只能是 png 格式，不宜过大'),
  );

  $form['fields']['file_image_watermark']['file'] = array(
    '#type' => 'hidden',
    '#default_value' => var_get_key('file_image_watermark', 'file', 0),
  );

  $form['fields']['file_image_watermark']['garble'] = array(
    '#type' => 'radio',
    '#required' => 1,
    '#title' => t('system', '开启混淆'),
    '#options' => array(t('system', '否'), t('system', '是')),
    '#default_value' => var_get_key('file_image_watermark', 'garble', 0),
    '#description' => t('system', '在图片上添加多个随机水印'),
  );
  
  $form['fields']['file_image_watermark']['Ugarble_file'] = array(
    '#type' => 'file',
    '#title' => t('system', '混淆水印'),
    '#description' => t('system', '只能是 png 格式，不宜过于清晰'),
  );
  
  $form['fields']['file_image_watermark']['garble_file'] = array(
    '#type' => 'hidden',
    '#default_value' => var_get_key('file_image_watermark', 'garble_file', 0)
  );

  $form['fields']['file_image_watermark']['min_width'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#title' => t('system', '最小宽度'),
    '#description' => t('system', '小于此宽度的图片不添加水印'),
    '#default_value' => var_get_key('file_image_watermark', 'min_width', 150)
  );
  
  $form['fields']['file_image_watermark']['min_height'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#title' => t('system', '最小高度'),
    '#description' => t('system', '小于此高度的图片不添加水印'),
    '#default_value' => var_get_key('file_image_watermark', 'min_height', 150)
  );
  
  $type = 'form';
  
  module_alter_all('system_site_files', $type, $form);
  
  $form['settings'] = array(
    '#validate' => array('system_site_files_validate'),
    '#submit' => 'system_site_files_submit',
  );
  
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '文件设置')));
  return system_setting_form($form);
}

function system_site_files_validate($form, &$v) {
  
  if ($files = file_get_array_data()) {
    $validate = array(
      'savepath' => 'watemark',
      'image' => array('type' => array('png')),
      'no_insert' => 1
    );
    
    if (!empty($files['file_image_watermark_Ufile'])) {
      if ($file = file_save_upload($files['file_image_watermark_Ufile'], $validate, 0)) {
        if (!empty($v['file_image_watermark']['file']) && $v['file_image_watermark']['file'] != $file->filepath) {
          @unlink($v['file_image_watermark']['file']);
        }
        $v['file_image_watermark']['file'] = $file->filepath;
      }
    }

    if (!empty($files['file_image_watermark_Ugarble_file'])) {
      if ($file = file_save_upload($files['file_image_watermark_Ugarble_file'], $validate, 0)) {
        if (!empty($v['file_image_watermark']['garble_file']) && $v['file_image_watermark']['garble_file'] != $file->filepath) {
          @unlink($v['file_image_watermark']['garble_file']);
        }
        $v['file_image_watermark']['garble_file'] = $file->filepath;
      }
    }
  }

  // 获取水印图片尺寸
  if (!empty($v['file_image_watermark']['file']) && $info = image_get_info($v['file_image_watermark']['file'])) {
    $v['file_image_watermark']['width'] = $info['width'];
    $v['file_image_watermark']['height'] = $info['height'];
  }

  if (!empty($v['file_image_watermark']['garble_file']) 
    && $info = image_get_info($v['file_image_watermark']['garble_file'])) {
    $v['file_image_watermark']['garble_width'] = $info['width'];
    $v['file_image_watermark']['garble_height'] = $info['height'];
  }

  return module_invoke_all('system_site_files', 'validate', $form, $v);
}

function system_site_files_submit($form, $v) {
  module_invoke_all('system_site_files', 'submit', $form, $v);
  file_mkdir($v['file_directory_path']);
}

/**
 * 已执行、待执行的计划任务列表
 */

/**
function system_admin_cron() {
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '计划任务')));
}
*/

/**
 * 默认首页内容
 */
function system_admin_front() {
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '首页设置')));
  $form['fields']['value'] = array(
    '#type' => 'textarea',
    '#title' => t('system', '首页默认内容'),
    '#default_value' => custom_get('front_default'),
    '#description' => t('system', '当首页没有数据时，将显示该内容')
  );
  $form['fields']['submit'] = array(
  	'#type' => 'submit',
    '#submit' => 'system_admin_front_submit'
  );
  dd_get_editor('value', array('toolbar' => 'medium', 'height' => 200));
  return $form;
}

function system_admin_front_submit($form, $v) {
  custom_set('front_default', $v['value']);
  dd_set_message(t('system', '操作成功'));
}

/**
 * 站点信息
 */
function system_site_global() {
  global $conf;
  
  $form['fields']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '站点名称'),
    '#required' => true,
    '#default_value' => var_get_key('site_global', 'name', t('system', '我的网站')),
  );
  $form['fields']['mail'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '站长邮箱'),
    '#required' => true,
    '#default_value' => var_get_key('site_global', 'mail'),
  );
  $form['fields']['send_mail'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '发件邮箱'),
    '#required' => true,
    '#default_value' => var_get_key('site_global', 'send_mail'),
    '#description' => t('system', '系统发出的信件上所显示的发件人')
  );
  $form['fields']['slogan'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '站点口号'),
    '#attributes' => array('size' => 50),
    '#description' => t('system', '显示在站点名称的后面'),
    '#default_value' => var_get_key('site_global', 'slogan'),
  );
  $form['fields']['description'] = array(
    '#type' => 'textarea',
    '#attributes' => array('rows' => 3),
    '#title' => t('system', '站点描述'),
    '#default_value' => var_get_key('site_global', 'description'),
    '#description' => t('system', '不支持 HTML，用于 RSS 或其它站点简介的地方，应尽量简洁')
  );
  $form['fields']['footer'] = array(
    '#type' => 'textarea',
    '#title' => t('system', '页脚信息'),
    '#default_value' => var_get_key('site_global', 'footer'),
    '#description' => t('system', '支持 HTML')
  );
  
  $type = 'form';
  
  module_alter_all('system_site_global', $type, $form);
  
  $form['settings'] = array(
    '#validate' => array('system_site_global_validate'),
  );
  
  $form['fields']['submit'] = array(
    '#submit' => 'system_site_global_submit',
    '#type' => 'submit'
  );
  
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '站点信息')));
  return $form;
}

function system_site_global_validate($form, $v) {
  return module_invoke_all('system_site_global', 'validate', $form, $v);
}

function system_site_global_submit($form, $v) {
  foreach ($form['fields'] as $key => $field) {
    if ($field['#type'] && !in_array($field['#type'], array('hidden', 'submit', 'button', 'file', 'captcha'))) {
     $data[$key] = $v[$key];
    }
  }
  var_set('site_global', $data, 0);
  module_invoke_all('system_site_global', 'submit', $form, $v);
  var_init();
}

/**
 * 站点配置
 */
function system_site_settings() {
	global $conf;
  
  $form['fields']['status'] = array(
    '#type' => 'radio',
    '#title' => t('system', '网站状态'),
    '#required' => true,
    '#default_value' => var_get('status', 1),
    '#options' => array(
      t('system', '关闭'),
      t('system', '开启'),
      t('system', '登录可访问'),
      t('system', '超级管理员可访问')
    ),
  );

  $form['fields']['default_language'] = array(
    '#type' => 'select',
    '#title' => t('system', '默认语言'),
    '#required' => true,
    '#default_value' => var_get('default_language', 'zh-hans'),
    '#options' => $conf['languages'],
  );

  $form['fields']['admin_language'] = array(
    '#type' => 'select',
    '#title' => t('system', '管理语言'),
    '#required' => true,
    '#default_value' => var_get('admin_language', 'zh-hans'),
    '#description' => t('system', '管理语言是指管理员在管理设置中输入的语言，如：站点名称、站点口号等，这些内容是动态的，必须知道它们的输入语言才能提供多语言支持(即生成语言翻译)。注意：这个设置是全站通用的，也就是说，在所有需要输入的站点设置选项内，必须输入同一种文字'),
    '#options' => $conf['languages'],
  );

  $form['fields']['clean_url'] = array(
    '#type' => 'radio',
    '#title' => t('system', '简洁链接'),
    '#required' => true,
    '#default_value' => var_get('clean_url', 1),
    '#options' => array(
      t('system', '关闭'),
      t('system', '开启'),
    ),
  );

  $form['fields']['default_host'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '域名限制'),
    '#default_value' => var_get('default_host', ''),
    '#description' => t('system', '若填写：didaah.org，则通过其它域名的访问，都将重定向，多个以半角逗号分隔')
  );

  $form['fields']['sql_info'] = array(
    '#type' => 'radio',
    '#title' => t('system', 'SQL执行语句'),
    '#required' => true,
    '#default_value' => var_get('sql_info', 0),
    '#options' => array(t('system', '不显示'), t('system', '显示'), t('system', '仅向超级管理员显示')),
  );

  $form['fields']['php_info'] = array(
    '#type' => 'radio',
    '#title' => t('system', 'PHP执行信息'),
    '#required' => true,
    '#default_value' => var_get('php_info', 1),
    '#options' => array(t('system', '不显示'), t('system', '显示'), t('system', '仅向超级管理员显示')),
  );

  $form['fields']['history_enabled'] = array(
    '#type' => 'radio',
    '#title' => t('system', '访问记录'),
    '#required' => true,
    '#default_value' => var_get('history_enabled', 0),
    '#options' => array(t('system', '不记录'), t('system', '记录')),
    '#description' => t('system', '记录网站的每一次访问')
  );

  $form['fields']['history_enabled_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '保留最新'),
    '#required' => true,
    '#default_value' => var_get('history_enabled_limit', 100000),
    '#field_suffix' => t('system', '条访问记录，0 为不限制')
  );

  $form['fields']['logs_enabled_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '保留最新'),
    '#required' => true,
    '#default_value' => var_get('logs_enabled_limit', 100000),
    '#field_suffix' => t('system', '条系统日志，0 为不限制')
  );

  $form['fields']['site_mode'] = array(
    '#type' => 'radio',
    '#title' => t('system', '开发模式'),
    '#required' => true,
    '#default_value' => var_get('site_mode', 1),
    '#description' => t('system', '开发模式将禁用某些缓存，以便于调试'),
    '#options' => array(t('system', '否'), t('system', '是')),
  );

  $form['fields']['admin_custom_links'] = array(
    '#type' => 'radio',
    '#title' => t('system', '管理区块快捷方式'),
    '#required' => true,
    '#default_value' => var_get('admin_custom_links', 1),
    '#options' => array(
      t('system', '禁止'),
      t('system', '允许'),
    ),
    '#description' => t('system', '允许拥有后台管理权限的用户添加一些快捷链接，供自己使用'),
  );

  $form['fields']['cache_css_and_js'] = array(
    '#type' => 'radio',
    '#title' => t('system', '合并CSS和JS'),
    '#required' => true,
    '#default_value' => var_get('cache_css_and_js', 0),
    '#options' => array(t('system', '不合并'), t('system', '合并')),
  );

  $form['fields']['filter_cache'] = array(
    '#type' => 'radio',
    '#title' => t('system', '缓存输入法'),
    '#required' => true,
    '#default_value' => var_get('filter_cache', 0),
    '#options' => array(t('system', '不缓存'), t('system', '缓存')),
    '#description' => t('system', '可以将通过输入法过滤的内容缓存到数据库，减少 PHP 开销，但将数据库将增多'),
  );

  $form['fields']['alias'] = array(
    '#type' => 'radio',
    '#title' => t('system', '别名优先'),
    '#required' => true,
    '#default_value' => var_get('alias', 0),
    '#options' => array(t('system', '否'), t('system', '是')),
    '#description' => t('system', '若存在别名，通过系统路径访问时，跳转到别名'),
  );

  $form['fields']['cron_limits'] = array(
    '#type' => 'textfield',
    '#title' => t('system', 'CRON 最大任务'),
    '#required' => true,
    '#default_value' => var_get('cron_limits', 100),
    '#description' => t('system', '通过 cron.inc 调度任务时，允许同时运行的最大任务数'),
  );

  $form['fields']['cron_min_time'] = array(
    '#type' => 'textfield',
    '#title' => t('system', 'CRON 时间间隔'),
    '#required' => true,
    '#default_value' => var_get('cron_min_time', 3600),
    '#description' => t('system', '两次 cron 间隔最小时间，以秒为单位，防止cron被频繁执行'),
  );

  $form['fields']['cron_pass'] = array(
    '#type' => 'textfield',
    '#title' => t('system', 'CRON 执行密码'),
    '#default_value' => var_get('cron_pass'),
    '#description' => t('system', '不设置请留空'),
  );

  $form['fields']['crypt_default_key'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '默认密钥'),
    '#default_value' => var_get('crypt_default_key', $GLOBALS['cookie_domain']),
    '#description' => t('system', '用于特定内容简单加密，如：cookie'),
  );

  $form['fields']['front_view_type'] = array(
    '#type' => 'select',
    '#title' => t('system', '网站首页'),
    '#required' => 1,
    '#default_value' => var_get('front_view_type'),
    '#options' => array(t('system', '系统默认'), t('system', '自定义')),
    '#description' => t('system', '若选择自定义，首页将不会有任何显示，仅传递可用变量，请自行在 front.tpl.php 中布局'),
  );
  
  $options = array(0 => t('system', '默认'));
  if ($modules = var_get('modules')) {
    foreach ($modules as $module => $info) {
      $options[$module] =  $info['name'];
    }
  }

  $form['fields']['front_view_module'] = array(
    '#type' => 'select',
    '#title' => t('system', '交给模块处理'),
    '#default_value' => var_get('front_view_module'),
    '#options' => $options,
    '#description' => t('system', '将网站首页完全交给某个模块处理，除非某个模块建议这么做，否则请不要选择'),
  );
  
  $type = 'form';
  
  module_alter_all('system_site_settings', $type, $form);
  
  // dd_get_editor('footer', array('toolbar' => 'medium', 'height' => 200));
  
  $form['settings'] = array(
    '#validate' => array('system_site_settings_validate'),
    '#submit' => 'system_site_settings_submit',
  );
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '站点设置')));
  return system_setting_form($form);
}

function system_site_settings_validate($form, $v) {
  if ($v['cache_css_and_js']) {
    file_mkdir(file_directory_path() . '/cache/css_js');
  }
  return module_invoke_all('system_site_settings', 'validate', $form, $v);
}

function system_site_settings_submit($form, $v) {
  module_invoke_all('system_site_settings', 'submit', $form, $v);
}

/**
 * 验证码设置
 */
function system_site_captcha() {

  dd_set_tabs(array(
    l(t('system', '验证码设置'), 'admin/settings/captcha'),
    l(t('system', '自定义验证码'), 'admin/settings/captcha/custom'),
  ));

  $form['fields']['captcha_file_ttf'] = array(
    '#type' => 'textfield',
    '#field_prefix' => DIDA_ROOT,
    '#default_value' => var_get('captcha_file_ttf', '/tools/simkai.ttf'),
    '#title' => t('system', '字体文件'),
    '#description' => t('system', '图片验证码必须提供中文字体文件，请将字体文件放于程序可访问的目录(推荐 tools)，在此设置路径')
  );

  if ($fields = module_invoke_all('system_site_captcha', 'fields')) {
    foreach ($fields as $key => $field) {
      if (!is_array($field)) continue;
      
      $value = var_get_key('captcha', $key);

      $form['fields'][$key]['data'] = array(
        '#type' => 'radio',
        '#title' => t('system', '启用'),
        '#required' => 1,
        '#default_value' => $value['data'] ? $value['data'] : 0,
        '#options' => array(
          t('system', '否'),
          t('system', '是'),
        ),
        '#fieldset_prefix' => 'asc',
        '#fieldset_legend' => $field['title'],
      );

      if (!empty($field['attributes']) && is_array($field['attributes'])) {
        $form['fields'][$key]['data'] = array_merge($form['fields'][$key]['data'], $field['attributes']);
      }

      $form['fields'][$key]['type'] = array(
        '#type' => 'select',
        '#title' => t('system', '类型'),
        '#default_value' => $value['type'],
        '#options' => array(
          t('system', '纯数字'),
          t('system', '数字、字母混合'),
          t('system', '自定义'),
          t('system', '计算式'),
          t('system', '问答式'),
        )
      );

      $form['fields'][$key]['image'] = array(
        '#type' => 'select',
        '#title' => t('system', '样式'),
        '#default_value' => $value['image'],
        '#options' => array(
          t('system', '文本'),
          t('system', '图片'),
        )
      );

      $form['fields'][$key]['mosaic'] = array(
        '#type' => 'select',
        '#title' => t('system', '干扰'),
        '#default_value' => $value['mosaic'],
        '#description' => t('system', '若为图片，是否加入干扰码，防止被识别'),
        '#options' => array(
          t('system', '关闭'),
          t('system', '开启'),
        )
      );

      $form['fields'][$key]['show'] = array(
        '#type' => 'select',
        '#title' => t('system', '显示'),
        '#default_value' => $value['show'],
        '#fieldset_suffix' => 1,
        '#description' => t('system', '可防止被识别'),
        '#options' => array(
          t('system', '需点击显示'),
          t('system', '默认显示'),
        )
      );
    }

    $form['fields']['submit'] = array(
      '#type' => 'submit',
      '#submit' => 'system_site_captcha_submit'
    );
  }

  $op = 'form';

  module_alter_all('system_site_captcha', $op, $form);

  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '验证码设置')));

  return $form;
}

function system_site_captcha_submit($form, $v) {
  var_set('captcha_file_ttf', $v['captcha_file_ttf'], 0);
  unset($v['captcha_file_ttf']);
  var_set('captcha', $v);
  dd_set_message(t('system', '保存成功'));
}

/**
 * 验证码设置
 */
function system_site_captcha_custom() {

  dd_set_tabs(array(
    l(t('system', '验证码设置'), 'admin/settings/captcha'),
    l(t('system', '自定义验证码'), 'admin/settings/captcha/custom'),
  ));

  $form = array('fields' => array());

  $form['fields']['ask'] = array(
    '#type' => 'textarea',
    '#default_value' => custom_get('_captcha_data_ask'),
    '#title' => t('system', '问答式验证码'),
    '#description' => t('system', '每行一个问题及答案，用户必须填写答案。以“|”分隔，如：中国有多少个民族？|56'),
  );

  $form['fields']['custom'] = array(
    '#type' => 'textarea',
    '#default_value' => custom_get('_captcha_data_custom'),
    '#title' => t('system', '自定义验证码'),
    '#description' => t('system', '每行一个验证码，用户必须填写相同内容。如：床前明月光'),
  );

  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#submit' => 'system_site_captcha_custom_submit',
  );

  return $form;
}

function system_site_captcha_custom_submit(&$form, &$v) {
  
  if (!empty($v['ask'])) {
    custom_set('_captcha_data_ask', $v['ask']);

    if ($data = dd_line_to_array($v['ask'], 1)) {
      $datas = array();
      foreach ($data as $value) {
        if (strpos($value, '|') !== false) {
          $datas[] = explode('|', $value, 2);
        }
      }
      if ($datas) custom_set('captcha_data_ask', $datas);
    }
  }

  if (!empty($v['custom'])) {
    custom_set('_captcha_data_custom', $v['custom']);
    if ($data = dd_line_to_array($v['custom'], 1)) {
      custom_set('captcha_data_custom', $data);
    }
  }

  dd_set_message(t('system', '操作成功'));
}

/**
 * 错误设置
 */
function system_site_error() {
  if ($image = var_get('image_error')) {
    $view = l(img($image, 
    t('system', '默认错误图片'), NULL, 
    array('width' => 100, 'height' => 100)), $image, array('html' => true));
  }
  $form['fields']['image_error'] = array(
    '#type' => 'file',
    '#title' => t('system', '错误图片'),
    '#description' => t('system', '当请求的图片不存在时，显示的图片') . $view,
    '#filesave' => true,// 系统处理上传文件
    '#validate' => array(
      'savepath' => 'error',
      'no_insert' => true,
      'title' => '默认错误图片',
      'filename' => 'image',
      'image' => array(
        'type' => array('jpg', 'png', 'gif'),
      ),
      'call_function' => '_system_image_error_save',
    ),
  );

  $form['fields']['debug'] = array(
    '#type' => 'radio',
    '#title' => t('system', '程序错误信息'),
    '#required' => true,
    '#default_value' => var_get('debug', 1),
    '#options' => array(
      t('system', '写入日志'),
      t('system', '写入日志并显示'),
      t('system', '写入日志并仅对超级管理员显示'),
    ),
  );
 
  $form['fields']['page_not_history'] = array(
    '#type' => 'radio',
    '#title' => t('system', '404错误'),
    '#required' => true,
    '#default_value' => var_get('page_not_history', 0),
    '#options' => array(
      t('system', '显示以下错误信息'),
      t('system', '返回上一页或首页'),
    )
  );
  
  $form['fields']['page_not_html'] = array(
    '#type' => 'textarea',
    '#title' => t('system', '404错误信息'),
    '#default_value' => custom_get('page_not_html', '当前页面不存在'),
    '#description' => t('system', '支持 HTML')
  );
  
  $form['fields']['page_access_history'] = array(
    '#type' => 'radio',
    '#title' => t('system', '403错误'),
    '#required' => true,
    '#default_value' => var_get('page_access_history', 0),
    '#options' => array(
      t('system', '显示以下错误信息'),
      t('system', '返回上一页或首页'),
    )
  );
  $form['fields']['page_access_html'] = array(
    '#type' => 'textarea',
    '#title' => t('system', '403错误信息'),
    '#default_value' => custom_get('page_access_html', '没有权限访问当前页面'),
    '#description' => t('system', '支持 HTML')
  );
  $form['fields']['site_offline_html'] = array(
    '#type' => 'textarea',
    '#title' => t('system', '站点维护信息'),
    '#default_value' => custom_get('site_offline_html', '站点维护中，请稍候访问…'),
    '#description' => t('system', '支持 HTML')
  );
  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#submit' => 'system_site_error_submit'
  );
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '错误设置')));
  return $form;
}

function system_site_error_submit($form, $v) {
  unset($v['image_error'], $v['submit'], $v['form_id'], $v['form_toke']);
  
  foreach ($v as $key => $value) {
    if ($key == 'page_access_html' || $key == 'page_not_html' || $key == 'site_offline_html') {
      custom_set($key, $value);
    } else {
      var_set($key, $value, 0);
    }
  }
  
  file_put_contents($GLOBALS['conf_dir'] . '/cache/site_offline_html.lang', $v['site_offline_html']);
  
  var_init();
}

/**
 * 默认错误图片上传成功后回调函数
 * @param $form_id
 * @param $name
 * @param $file
 */
function _system_image_error_save($form_id, $name, $file) {
  var_set('image_error', $file->filepath);
}

function _system_cache_htaccess() {
  $filepath = $GLOBALS['conf_dir'].'/cache/.htaccess';
  if (!is_file($filepath)) {
    $h = "#禁止下载\n";
    $h .= '<FilesMatch "\.*$">';
    $h .= "\nOrder allow,deny\n";
    $h .= '</FilesMatch>';
    @file_put_contents($filepath, $h);
    @chmod($filepath, 0644);
  }
}

/**
 * 缓存设置
 */
function system_cache_settings() {
  dd_set_tabs(array(
    l(t('system', '缓存管理'), 'admin/cache'),
    l(t('system', '缓存设置'), 'admin/cache/settings'),
  ));

  $options = array();
  
  $options['/includes/cache.inc'] = t('system', '系统默认');
  
  if (is_dir(DIDA_ROOT . '/includes/cache')) {
    if ($objs = glob(DIDA_ROOT . '/includes/cache/*')) {
      foreach ($objs as $obj) {
        if (strpos($obj, '.inc') !== false) {
          $name = basename($obj);
          $options['/includes/cache/' . $name] = $name;
        }
      }
    }
  }
  
  $form['fields']['dd_cache_file'] = array(
    '#title' => t('system', '缓存方式'),
    '#type' => 'select',
    '#required' => 1,
    '#default_value' => var_get('dd_cache_file', '/includes/cache.inc'),
    '#options' => $options,
  );
  
  if (function_exists('cache_settings_custom_form')) {
    cache_settings_custom_form($form);
  }
  
  return system_setting_form($form);
}

/**
 * @Implement of hook_system_cache_clear()
 */
function system_system_cache_clear($op = 'list', $v = NULL) {
  switch ($op) {
    case 'list':
    return array(
      'modules' => t('system', '清除模块信息缓存，当模块 info 信息变动时应选择此项'),
      'perms' => t('system', '清除用户权限缓存，当有新的权限注册时应选择此项'),
      'themes' => t('system', '清除主题信息缓存，当主题 info 信息变动时应选择此项'),
      'menu' => t('system', '清除menu信息缓存，当有模块注册新的 url 或旧的 url 权限变动时应选择此项'),
      'cache_menu' => t('system', '清除cache_menu缓存表，当旧的 url 权限变动时应选择此项'),
      'admin_menus' => t('system', '清除管理菜单缓存表，当有新的管理菜单注册时应选择此项'),
      'cache_css_and_js' => t('system', '清除js和css缓存'),
    	'cache_hook' => t('system', '清除 hook 缓存，当有模块变动时应选择此项'),
      'cache_filter' => t('system', '清除 filter 缓存，当有输入法或过滤器有变动时应选择此项'),
    );
    // case 'submit':
  }
}

/**
 * 缓存管理
 */
function system_cache_clear() {
  dd_set_tabs(array(
    l(t('system', '缓存管理'), 'admin/cache'),
    l(t('system', '缓存设置'), 'admin/cache/settings'),
  ));
  $form['fields']['value'] = array(
    '#type' => 'checkbox',
    '#options' => module_invoke_all('system_cache_clear', 'list'),
  );
  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#field_prefix' => form_util_all_check(array('alt' => 'system_cache_clear_type_value')),
    '#value' => t('system', '清除缓存'),
    '#submit' => 'system_cache_clear_submit',
  );
  return $form;
}

function system_cache_clear_submit($form, $value) {
  cache_del('cid', 'element_func');
  if ($value['value']['modules']) {
    module_set_list('module', 'modules');
    module_enabled_variable('module');
    cache_del('cid', 'schema');
    dd_set_message(t('system', '模块信息缓存清除成功'));
  }
  if ($value['value']['themes']) {
    module_set_list('theme', 'themes');
    module_enabled_variable('theme');
    dd_set_message(t('system', '主题信息缓存清除成功'));
  }
  if ($value['value']['perms']) {
    cache_del('cid', 'user_perm');
    dd_set_message(t('system', '用户权限缓存清除成功'));
  }
  if ($value['value']['menu']) {
    menu_set_item();
    dd_set_message(t('system', 'menu 信息缓存清除成功'));
  }
  if ($value['value']['cache_hook']) {
    module_hook_set_cache();
    theme_get_cache();
    dd_set_message(t('system', 'hook 缓存清除成功'));
  }
  if ($value['value']['cache_filter']) {
    filter_set_cache();
    dd_set_message(t('system', 'filter 缓存清除成功'));
  }
  if ($value['value']['cache_menu']) {
    cache_del('empty', NULL, 'cache_menu');
    dd_set_message(t('system', 'menu 列表缓存清除成功'));
  }
  if ($value['value']['cache_css_and_js']) {
    $filepath = file_directory_path() . '/cache/css_js';
    if (is_dir($filepath)) {
      if ($objs = glob($filepath.'/*')) {
        foreach ($objs as $obj) {
          is_dir($obj) ? @rmdir($obj) : @unlink($obj);
        }
      }
    }
    var_set('cache_css_and_js_timestamp', $_SERVER['REQUEST_TIME'], 0);
    dd_set_message(t('system', 'js和css缓存清除成功'));
  }
  if ($value['value']['admin_menus']) {
    cache_del('*', 'admin_menus_');
    dd_set_message(t('system', '管理菜单缓存清除成功'));
  }
  
  _system_cache_htaccess();
  
  module_invoke_all('system_cache_clear', 'submit', $value);
  var_init();
}

/**
 * 通配符
 */
function system_admin_token() {
	if ($tokens = _module_invoke_all('token_list')) {
		foreach ($tokens as $module => $lists) {
			$table[][] = array('data' => dd_get_strong($module), 'colspan' => 2);
			foreach ($lists as $key => $info) {
				$table[] = array($key, $info['title']);
			}
		}
		return theme('table', array(t('system', '通配符'), t('system', '替换为')), $table);
	}
}

function system_admin_lang() {
	global $conf;
  dd_set_tabs(array(l(t('system', '语言管理'), 'admin/lang'), l(t('system', '导入语言'), 'admin/lang/cache')));
  dd_set_help(t('system', '所有语言文件将被合并，保存在 %path 目录下', array('%path' => $GLOBALS['conf_dir'].'/cache/languages')));
	if (arg(2)) {
    if ($conf['languages'][arg(2)]) {
  		$lang = arg(2);
  		switch (arg(3)) {
  			case 'edit':
  				$edit = array('language' => $lang, 'name' => $conf['languages'][$lang]);
  			break;
  			case 'delete':
  				if ($lang != $conf['default_language']) {
  					db_exec('DELETE FROM {languages} WHERE language = ?', array($lang));
  					cache_del_file('lang_' . $lang . '.lang');
  					system_lang_cache();
  					dd_set_message(t('system', '删除成功'));
  				} else {
  					dd_set_message(t('system', '默认语言不能删除'));
  				}
  				dd_goto('admin/lang');
  				exit;
  		}
    } else if (arg(2) == 'cache') {
      return dd_get_form('system_admin_lang_cache');
    }
	}
  
  $output = dd_get_form('system_admin_lang_form', $edit);
  
  if ($conf['languages']) {
  	foreach ($conf['languages'] as $key => $name) {
  		$links = array(l(t('system', '编辑'), 'admin/lang/' . $key . '/edit'));
  		if ($conf['default_language'] == $key) {
  			$key .= t('system', '(当前默认语言)');
  		} else {
  			$links[] =  l(t('system', '删除'), 'admin/lang/' . $key . '/delete');
  		}
  		$table[] = array($key, $name, dd_imp($links));
  	}
  }
  
  $output .= theme('table', array(t('system', '语言代码'), t('system', '语言名称'), t('system', '操作')), $table);
  
	return $output;
}

// 语言设置
function system_admin_lang_form($lang = NULL) {
	global $conf;
  
	$form['settings']['#validate'] = array('system_admin_lang_form_validate');
  $form['fields']['language'] = array(
    '#type' => 'select',
    '#title' => t('system', '添加语言'),
    '#required' => 1,
    '#options' => system_get_languages_data(),
    '#default_value' => $lang['language'],
  );
  $form['fields']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('system', '语言名称'),
    '#default_value' => $lang['name'],
    '#required' => 1,
    '#description' => t('system', '用户看到的语言名称')
  );
  
  if ($lang['language']) {
    $form['fields']['update'] = array(
      '#type' => 'hidden',
      '#default_value' => $lang['language'],
      '#constant' => 1
    );
  }
  
  $form['fields']['submit']['#type'] = 'submit';
	return $form;
}

function system_admin_lang_form_validate($form, $v) {
	$v['language'] = trim($v['language']);
	$v['name'] = trim($v['name']);
	if (mb_strlen($v['language']) > 16) {
		$t['language']['#error'][] = t('system', '长度不超过 16 字符');
	} else if (mb_strlen($v['name']) > 32) {
		$t['name']['#error'][] = t('system', '长度不超过 32 字符');
	} else {
		if (!$v['update']) {
			if (db_query('SELECT COUNT(*) FROM {languages} WHERE language = ? OR name = ?', array($v['language'], $v['name']), array('return' => 'column'))) {
				$t['name']['#error'][] = t('system', '该语言已经存在');
			} else {
				db_exec('INSERT INTO {languages} (language, name) VALUES (?, ?)', array($v['language'], $v['name']));
				
				system_lang_cache();
				
				dd_set_lang(NULL, array($v['language'])); // 导入语言文件
				
				dd_set_message(t('system', '%string 添加成功', array('%string' => $v['name'])));
			}
		} else {
			db_exec('UPDATE {languages} SET language = ?, name = ? WHERE language = ?', array($v['language'], $v['name'], $v['update']));
			if ($v['language'] != $v['update']) {
				cache_del_file('lang_' . $v['update'] . '.php');
				if ($v['update'] == $GLOBALS['conf']['default_language']) {
					var_set('default_language', $v['language']);
				}
				dd_set_lang(NULL, array($v['language'])); // 导入语言文件
			}
			
			system_lang_cache();
			
			dd_goto('admin/lang');
		}
	}
	return $t;
}

/**
 * 语言文件更新
 */
function system_admin_lang_cache() {
	global $conf;
	$form['settings']['#validate'] = array('system_admin_lang_cache_validate');
  
  $form['fields']['language'] = array(
  	'#type' => 'select',
  	'#title' => t('system', '选择语言'),
  	'#options' => $conf['languages'],
    '#description' => '请备份你修改过的语言缓存文件',
  	'#required' => 1,
  );
  
  $form['fields']['submit']['#type'] = 'submit';
  
	return $form;
}

function system_admin_lang_cache_validate($form, $v) {
	if ($v['language']) {
		$languages[] = $v['language'];
	} else {
		$languages = NULL;
  }

	dd_set_lang(NULL, $languages);
}

/**
 * 输入设置
 */
function system_admin_filter() {
  global $conf;
  
  $item = array(
    l(t('system', '输入法列表'), 'admin/filter'),
    l(t('system', '过滤器列表'), 'admin/format'),
    l(t('system', '添加输入法'), 'admin/filter/add', dd_redirect(url('admin/filter'))),
    l(t('system', '添加过滤器'), 'admin/format/add'),
  );
  $args = arg();
  switch ($args[2]) {
    case 'config':
      $item[] = l(t('system', '输入法配置'), $_GET['q']);
      $output .= dd_get_form('system_admin_filter_form', $args[3]);
    break;
    case 'add':
      $output .= dd_get_form('system_admin_filter_form');
    break;
    case 'delete':
      if (db_exec('DELETE FROM {filter_format} WHERE format = ?', array($args[3]))) {
        filter_set_cache(); // 清除配置文件缓存
        dd_set_message(t('system', '删除成功'));
      } else {
        dd_set_message(t('system', '删除失败'));
      }
      dd_goto('admin/filter');
    exit;
    case 'default':
      if ($conf['default_format'] != $args[3]) {
        if (db_query('SELECT format FROM {filter_format} WHERE format = ?', array($args[3]), array('return' => 'column'))) {
          var_set('default_format', $args[3]);
          dd_set_message(t('system', '操作成功'));
        }
      }
      dd_goto('admin/filter');
    exit;
    default:
      if ($fetch = db_query('SELECT * FROM {filter_format}')) {
        foreach ($fetch as $o) {
          $links = array(l(t('system', '设置'), 'admin/filter/config/'.$o->format), l(t('system', '删除'), 'admin/filter/delete/'.$o->format, array('attributes' => array('class' => 'confirm'))));
          if ($conf['default_format'] != $o->format) {
            $links[] = l(t('system', '设为默认'), 'admin/filter/default/'.$o->format);
          }
          
          $filters[] = $o->name . dd_imp($links) . '<span class="description form_description">'. check_plain($o->description). '</span>';
        }
      }
      $output .= theme('item_list', $filters, t('system', '输入法列表'));
  }
  
  dd_set_tabs($item);
  
  $output = $output;
  
  return $output;
}

/**
 * 过滤器列表
 */
function system_admin_format() {
  global $conf;
  
  $item = array(
    l(t('system', '输入法列表'), 'admin/filter'),
    l(t('system', '过滤器列表'), 'admin/format'),
    l(t('system', '添加输入法'), 'admin/filter/add', dd_redirect(url('admin/filter'))),
    l(t('system', '添加过滤器'), 'admin/format/add'),
  );
  $args = arg();
  switch ($args[2]) {
    case 'add':
      if ($lists = _module_invoke_all('filter', 'add')) {
        foreach ($lists as $module => $links) {
          foreach ($links as $delta => $link) {
            $filters[] = l($link['title'], 'admin/format/module/form/'.$module.'/'.$delta, dd_redirect(url('admin/format'))) . '<span class="description form_description">'. check_plain($link['description']). '</span>';
          }
        }
        $output .= theme('item_list', $filters);
      } else {
        dd_set_message(t('system', '没有可用的项目'));
      }
    break;
    case 'module':
      switch ($args[3]) {
        case 'form':
          // 添加过滤器
          if ($args[4]) {
            $output .= dd_get_form('system_admin_format_form', $args[4], $args[5]);
          } else {
            dd_set_message(t('system', '没有可用的项目'));
          }
        break;
        case 'update':
          if ($filter = db_query('SELECT * FROM {filter_tags} WHERE fid = ?', array($args[4]), array('return' => 'one'))) {
            $output .= dd_get_form('system_admin_format_form', $filter->module, $filter->delta, $filter);
          } else {
            dd_set_message(t('system', '不正确的访问'));
            dd_goto('admin/format', NULL, NULL, NULL, 1);
          }
        break;
        case 'delete':
          // 删除过滤器
          if ($filter = db_query('SELECT * FROM {filter_tags} WHERE fid = ?', array($args[4]), array('return' => 'one'))) {
            db_exec('DELETE FROM {filter_tags} WHERE fid = ?', array($args[4]));
            db_exec('DELETE FROM {filters} WHERE filter = ?', array($filter->fid));
            if (function_exists($filter->module.'_filter')) {
              call_user_func($filter->module.'_filter', 'delete', $filter->delta, NULL, $filter);
            }
            
            filter_set_cache(); // 清除配置文件缓存
            cache_del('cid', 'filter_get_filters'); // 清除自定义过滤器缓存
            dd_set_message(t('system', '清除成功'));
          } else {
            dd_set_message(t('system', '清除失败'));
          }
          dd_goto_redirect('admin/format');
      }
    break;
    default:
      if ($lists = _module_invoke_all('filter', 'list')) {
        
        $allLists = array();
        foreach ($lists as $module => $filters) {
          foreach ($filters as $delta => $filter) {
            $filter['delta'] = $delta;
            $filter['module'] = $module;
            $allLists[$delta.'_'.$module] = (object)$filter;
          }
        }
        foreach ($allLists as $key => $filter) {
          $links = array();
          if ($filter->update) {
            if ($filter->update == 1) {
              $links[] = l(t('system', '设置'), 'admin/format/module/update/'.$filter->filter_id, dd_redirect());
            } else {
              $links[] = l(t('system', '设置'), $filter->update.'/'.$filter->delta, dd_redirect());
            }
          }
          if ($filter->delete) {
            if ($filter->delete == 1) {
              $links[] = l(t('system', '清除'), 'admin/format/module/delete/'.$filter->filter_id, dd_redirect());
            } else {
              $links[] = l(t('system', '清除'), $filter->delete.'/'.$filter->delta, dd_redirect());
            }
          }
          $filters[] = $filter->title . dd_imp($links) . '<span class="description form_description">'. check_plain($filter->description). '</span>';
        }
        $output .= theme('item_list', $filters, t('system', '过滤器列表'));
      }
  }
  
  dd_set_tabs($item);
  
  $output = $output;
  return $output;
}

/**
 * 过滤器表单
 */
function system_admin_format_form($module, $delta, $filter = NULL) {
  
  $add = call_user_func($module.'_filter', 'add', $delta);
  if (!$add[$delta] || !$add[$delta]['title']) {
    dd_set_message(t('system', '不正确的访问'));
    dd_goto('admin/filte/add_filter');
    exit;
  }
  $form['fields']['title'] = array(
    '#title' => t('system', '名称'),
    '#type' => 'textfield',
    '#required' => 1,
    '#default_value' => $filter->title,
    '#description' => t('system', '不超过 %length 字符', array('%length' => 60))
  );
  $form['fields']['description'] = array(
    '#title' => t('system', '描述'),
    '#type' => 'textfield',
    '#attributes' => array('size' => 60),
    '#default_value' => $filter->description,
    '#description' => t('system', '不超过 %length 字符', array('%length' => 255))
  );
  $form['fields']['fid'] = array(
    '#type' => 'hidden',
    '#default_value' => $filter->fid,
    '#constant' => true,
  );
  $form['fields']['module'] = array(
    '#type' => 'hidden',
    '#default_value' => $module,
    '#constant' => true,
  );
  $form['fields']['delta'] = array(
    '#type' => 'hidden',
    '#default_value' => $delta,
    '#constant' => true,
  );
  if ($k = call_user_func($module.'_filter', 'form', $delta, NULL, $filter)) {
    $form['fields'] += $k;
  }
  $form['settings'] = array(
    '#validate' => array('system_admin_format_form_validate'),
    '#title' => $add[$delta]['title']
  );
  $form['fields']['submit']['#type'] = 'submit';
  return $form;
}

function system_admin_format_form_validate($form, $v) {
  if (mb_strlen($v['title']) > 60) {
    $t['title']['#error'][] = t('system', '不超过 %length 字符', array('%length' => 60));
  } else if (mb_strlen($v['description']) > 255) {
    $t['description']['#error'][] = t('system', '不超过 %length 字符', array('%length' => 255));
  } else if (!$t = call_user_func($v['module'].'_filter', 'validate', $v['delta'], NULL, (object)$v)) {
    if ($v['fid']) {
      // 编辑
      if (!db_query('SELECT title FROM {filter_tags} WHERE title = ? AND fid != ?', array($v['title'], $v['fid']), array('return' => 'column'))) {
        db_exec('UPDATE {filter_tags} SET title = ?, description = ? WHERE fid = ?', array($v['title'], $v['description'], $v['fid']));
        dd_set_message(t('system', '编辑成功'));
      } else {
        $t['title']['#error'][] = t('system', '%string 已经存在', array('%string' => $v['title']));
      }
    } else if (!db_query('SELECT title FROM {filter_tags} WHERE title =?', array($v['title']), array('return' => 'column'))) {
      // 添加
      if (db_exec('INSERT INTO {filter_tags} (title, module, delta, description) VALUES (?, ?, ?, ?)', array($v['title'], $v['module'], $v['delta'], $v['description']))) {
        $v['fid'] = db_last_insert_id();
        dd_set_message(t('system', '保存成功'));
      } else {
        dd_set_message(t('system', '保存失败'));
        return true;
      }
    } else {
      $t['title']['#error'][] = t('system', '%string 已经存在', array('%string' => $v['title']));
    }
    call_user_func($v['module'].'_filter', 'save', $v['delta'], NULL, (object)$v);
    cache_del('cid', 'filter_get_filters'); // 清除自定义过滤器缓存
  }
  return $t;
}

/**
 * 输入法配置
 */
function system_admin_filter_form($fid = NULL) {
  if ($fid && (!$format = db_query('SELECT * FROM {filter_format} WHERE format = ?', array($fid), array('return' => 'one')))) {
    dd_set_message(t('system', '不正确的访问'));
    dd_goto('admin/filter');
    exit;
  }
  
  if ($format->name) {
    dd_set_title(array(t(0, $format->name), t('system', '输入法配置'), t('system', '输入设置')));
  } else {
    dd_set_title(array(t('system', '添加输入法'), t('system', '输入设置')));
  }
  
  if ($format->roles) {
    $format->roles = unserialize($format->roles);
  } else {
    $format->roles = array();
  }
  $form['fields']['name'] = array(
    '#title' => t('system', '名称'),
    '#type' => 'textfield',
    '#required' => 1,
    '#default_value' => $format->name,
    '#description' => t('system', '不超过 %length 字符', array('%length' => 60))
  );
  $form['fields']['description'] = array(
    '#title' => t('system', '描述'),
    '#type' => 'textfield',
    '#attributes' => array('size' => 60),
    '#default_value' => $format->description,
    '#description' => t('system', '不超过 %length 字符', array('%length' => 255))
  );
  if ($lists = _module_invoke_all('filter', 'list')) {
    $oldLists = $allLists = array();
    
    foreach ($lists as $module => $filters) {
      foreach ($filters as $delta => $filter) {
        $filter['delta'] = $delta;
        $filter['module'] = $module;
        $allLists[$delta.'_'.$module] = (object)$filter;
      }
    }
    if ($fid) {
      if ($fetch = db_query('SELECT * FROM {filters} WHERE format = ? ORDER BY weight ASC, fid DESC', array($fid))) {
        foreach ($fetch as $o) {
          $key = $o->delta.'_'.$o->module;
          if (!empty($allLists[$key])) {
            $allLists[$key]->fid = $o->fid;
            $allLists[$key]->weight = $o->weight;
          }
        }
      }
  	  $form['fields']['fid'] = array(
  	    '#type' => 'hidden',
  	    '#default_value' => $fid,
  	    '#constant' => true,
  	  );
    }
    $form['fields']['roles'] = array(
      '#type' => 'checkbox',
      '#title' => t('system', '角色'),
      '#default_value' => $format->roles,
      '#description' => t('system', '允许使用此输入法'),
      '#options' => user_roles_opt()
    );
    $form['fields']['filter'] = array(
	    '#fieldset_prefix' => 'asc',
	    '#fieldset_legend' => t('system', '过滤器列表'),
      '#fieldset_suffix' => 1,
      '#description' => t('system', '权重将决定过滤器调用的先后顺序，多个过滤器之间可能存在冲突')
    );
    foreach ($allLists as $key => $filter) {
      $op = array();
      if ($filter->update) {
        if ($filter->update == 1) {
          $op[] = l(t('system', '设置'), 'admin/format/module/update/'.$filter->filter_id, dd_redirect());
        } else {
          $op[] = l(t('system', '设置'), $filter->update.'/'.$filter->delta, dd_redirect());
        }
      }
      if ($filter->delete) {
        if ($filter->delete == 1) {
          $op[] = l(t('system', '清除'), 'admin/format/module/delete/'.$filter->filter_id, dd_redirect());
        } else {
          $op[] = l(t('system', '清除'), $filter->delete.'/'.$filter->delta, dd_redirect());
        }
      }
      
      $form['fields']['filter'][$key] = array(
        '#fieldset_prefix' => 'asc',
        '#fieldset_legend' => t('system', $filter->title),
        '#fieldset_suffix' => 1,
        '#description' => dd_imp($op) . check_plain($filter->description),
        '#fieldset' => true,
      );

      $form['fields']['filter'][$key]['module'] = array(
        '#type' => 'hidden',
        '#constant' => 1,
        '#default_value' => $filter->module,
      );

      $form['fields']['filter'][$key]['delta'] = array(
        '#type' => 'hidden',
        '#constant' => 1,
        '#default_value' => $filter->delta,
      );

      $form['fields']['filter'][$key]['filter'] = array(
        '#type' => 'hidden',
        '#constant' => 1,
        '#default_value' => $filter->filter_id ? $filter->filter_id : 0,
      );
      
      $form['fields']['filter'][$key]['check'] = array(
        '#title' => t('system', '启用'),
        '#prefix' => '<div>',
        '#type' => 'checkbox',
        '#default_value' => $filter->fid ? 1 : 0,
      );

      $form['fields']['filter'][$key]['weight'] = array(
        '#title' => t('system', '权重'),
        '#type' => 'weight',
        '#start' => -30,
        '#suffix' => '</div>',
        '#default_value' => $filter->weight ? $filter->weight : 0,
        '#end' => 30,
      );
    }
  }
  $form['settings']['#validate'] = array('system_admin_filter_form_validate');
  $form['fields']['submit']['#type'] = 'submit';
  
  return $form;
}

function system_admin_filter_form_validate($form, $v) {
  if (mb_strlen($v['name']) > 60) {
    $t['name']['#error'][] = t('system', '不超过 %length 字符', array('%length' => 60));
  } else if (mb_strlen($v['description']) > 255) {
    $t['description']['#error'][] = t('system', '不超过 %length 字符', array('%length' => 255));
  } else if ($v['fid']) {
    if (!db_query('SELECT name FROM {filter_format} WHERE name = ? AND format != ?', array($v['name'], $v['fid']), array('return' => 'column'))) {
      db_exec('UPDATE {filter_format} SET name = ?, description = ?, roles = ? WHERE format = ?', array($v['name'], $v['description'], serialize($v['roles']), $v['fid']));
      db_exec('DELETE FROM {filters} WHERE format = ?', array($v['fid']));
      $fid = $v['fid'];
      dd_set_message(t('system', '编辑成功'));
    } else {
      $t['name']['#error'][] = t('system', '%string 已经存在', array('%string' => $v['name']));
    }
  } else if (!db_query('SELECT name FROM {filter_format} WHERE name =?', array($v['name']), array('return' => 'column'))) {
    if (db_exec('INSERT INTO {filter_format} (name, description, roles) VALUES (?, ?, ?)', array($v['name'], $v['description'], serialize($v['roles'])))) {
      $fid = db_last_insert_id();
      dd_set_message(t('system', '添加成功'));
    }
  } else {
    $t['name']['#error'][] = t('system', '%string 已经存在', array('%string' => $v['name']));
  }
  
  if ($fid) {
    if (!empty($v['filter'])) {
      foreach ($v['filter'] as $key => $filter) {
        if (is_array($filter) && !empty($filter['check'])) {
          if (empty($filter['filter_id'])) $filter['filter_id'] = 0;
          $filter['format'] = $fid;
          db_write_record('filters', $filter); 
        }
      }
    }
    filter_set_cache();
  }
  
  return $t;
}

/**
 * 别名管理
 */
function system_admin_alias() {
  
  dd_set_tabs(array(
    l(t('system', '别名管理'), 'admin/alias'),
    l(t('system', '添加别名'), 'admin/alias/add'),
  ));
  
  switch (arg(2)) {
    case 'add':
      $output = dd_get_form('system_admin_alias_form');
    break;
    
    case 'delete':
      if ($_GET['alias'] && db_exec('DELETE FROM {alias} WHERE alias = ?', array($_GET['alias']))) {
        echo 'tr';
      }
    exit;
    
    default:
      
      $header = array(
        t('system', '路径'),
        t('system', '别名'),
        array('data' => t('system', '扩展类型'), 'field' => 'type'),
        t('system', '删除')
      );
      
      if ($fetch = pager_query('SELECT * FROM {alias}' . table_sql($header), 50)) {
        foreach ($fetch as $o) {
          $table[] = array(
            $o->path, l($o->alias, $o->alias), $o->type,
            l(t('system', '删除'), 'admin/alias/delete', array(
              'query' => 'alias=' . $o->alias,
              'attributes' => array(
                'class' => 'confirmajax'
              )
            ))
          );
        }
        
        $output = theme('table', $header, $table, array('class' => 'center_table'));
        $output .= pager_view();
        
      }
  }
  
  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '别名管理')));
  return $output;
}

/**
 * 别名添加表单
 */
function system_admin_alias_form() {
  $form['fields']['type'] = array(
    '#type' => 'hidden',
    '#default_value' => 'system',
    '#constant' => 1
  );
  
  $form['fields']['tid'] = array(
    '#type' => 'hidden',
    '#default_value' => 0,
    '#constant' => 1
  );
  
  $form['fields']['path'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#title' => t('system', '路径'),
    '#description' => t('system', '只能是系统路径，如：user/login')
  );
  
  $form['fields']['alias'] = array(
    '#type' => 'textfield',
    '#required' => 1,
    '#title' => t('system', '别名'),
    '#description' => t('system', '不超过 255 字符，如：login.shtml')
  );
  
  $form['fields']['button'] = array(
    '#type' => 'submit',
    '#submit' => 'system_admin_alias_form_submit'
  );
  
  $form['settings'] = array(
    '#validate' => array('system_admin_alias_form_validate'),
    '#ajax_validate' => 1
  );
  
  return $form;
}

function system_admin_alias_form_validate($form, $v) {
  
  if (mb_strlen($v['alias'], 'utf-8') > 255) {
    $t['alias']['#error'][] = t('system', '别名不大于 250 字符');
  } else if (menu_check_reservation($v['alias'])) {
    $t['alias']['#error'][] = t('system', '别名在受限的列表中，请换一个试试');
  } else if (menu_get_path($v['alias'])) {
    $t['alias']['#error'][] = t('system', '%string 已经存在', array('%string' => $v['alias']));
  }
  
  return $t;
}

function system_admin_alias_form_submit($form, $v) {
  menu_set_alias($v['path'], $v['alias'], $v['type'], $v['tid']);
  dd_set_message(t('system', '添加成功'));
  dd_goto('admin/alias');
}

// public system_admin_sql() {{{ 
/**
 * system_admin_sql
 *  执行 SQL
 * 
 * @access public
 * @return array
 */
function system_admin_sql() {
  global $disable_admin_sql_update, $database;

  if (!empty($_GET['ajax'])) {
    if (!empty($_POST['data'])) {
      $_POST['data'] = trim($_POST['data']);
      if (!empty($database[$_POST['target']])) {
        $target = $_POST['target'];
      } else {
        $target = 'default';
      }

      if (strpos($_POST['data'], ";\n") !== false) {
        $query = explode(";\n", $_POST['data']);
      } else {
        $query = array($_POST['data']);
      }

      switch ($_POST['type']) {
        case 1: case 2:
          $item = array();
          if (!empty($disable_admin_sql_update)) {
            foreach ($query as $sql) {
              $count = db_exec($sql, array(), array('target' => $target));
              $item[] = '<strong>' . $sql . '</strong>';
              $item[] = t('system', '受影响的行 !count', array('!count' => $count));
            }
          } else {
            $item[] = t('system', '要使用此功能，请在 setting.php 中加入如下代码：');
            $item[] = 'global $disable_admin_sql_update;';
            $item[] = '$disable_admin_sql_update = true;';
          }
          $output = theme('item_list', $item);
        break;
        default:
          $output = '';
          foreach ($query as $sql) {
            // 若没有设置行数，默认限定为 100 条
            if (stripos($sql, 'select') !== false && stripos($sql, 'limit') === false) {
              $sql = rtrim(trim($sql), ';') . ' LIMIT 0, 100';
            }
            if ($fetch = db_query($sql, array(), array('target' => $target))) {
              $header = array();
              $output .= '<strong>' . $sql . '</strong>';
              $table = array();
              foreach ($fetch as $i => $o) {
                $rows = array();
                foreach ($o as $key => $v) {
                  if ($i == 0) $header[] = $key;
                  $rows[] = check_plain($v);
                }
                $table[] = $rows;
              }
              $output .= theme('table', $header, $table);
            }
          }
      }
      echo '$("#system_admin_sql_output_value").html("' . addslashes(str_replace(array("\n", "\r"), '', $output)) . '");';
    }
    exit;
  }

  dd_set_breadcrumb(array(l(t('system', '系统管理'), 'admin'), t('system', '执行 SQL')));

  $form = array(
    'fields' => array(),
    'settings' => array(
      '#ajax_validate' => true,
      '#ajax_submit' => array(
        'options' => array(
          'dataType' => 'script',
          'url' => url('admin/sql', array('query' => array('ajax' => 1))),
        ),
      ),
    )
  );

  $form['fields']['data'] = array(
    '#title' => t('system', 'SQL 语句'),
    '#type' => 'textarea',
    '#description' => t('system', '多条以分号分隔'),
    '#required' => 1,
  );

  $form['fields']['type'] = array(
    '#title' => t('system', '语句类型'),
    '#type' => 'radio',
    '#required' => 1,
    '#default_value' => 0,
    '#description' => t('system', '系统不会过滤语句，该选项仅用于确认操作，请谨慎'),
    '#options' => array(
      t('system', '查询'),
      t('system', '更新'),
      t('system', '删除'),
    )
  );
 
  $keys = array_keys($database);

  $form['fields']['target'] = array(
    '#title' => t('system', '选择连接'),
    '#type' => 'select',
    '#required' => 1,
    '#default_value' => 0,
    '#description' => t('system', '选择一个数据库连接，通常使用 default'),
    '#options' => array_combine($keys, $keys),
  );

  $form['fields']['button'] = array(
    '#type' => 'submit',
  );
 
  $form['fields']['output'] = array(
    '#type' => 'value',
    '#title' => t('system', '运行结果'),
    '#value' => '<div id="system_admin_sql_output_value"></div>'
  );

  return $form;
}
// }}}

/**
 * @reference
 *  http://people.w3.org/rishida/names/languages.html
 */
function system_get_languages_data($lang = NULL) {
  $languages = array(
    'aa' => 'Afar',
    'ab' => 'Abkhazian(аҧсуа бызшәа)',
    'ae' => 'Avestan',
    'af' => 'Afrikaans',
    'ak' => 'Akan',
    'am' => 'Amharic(አማርኛ)',
    'ar' => 'Arabic(العربية)',
    'as' => 'Assamese',
    'av' => 'Avar',
    'ay' => 'Aymara',
    'az' => 'Azerbaijani(azərbaycan)',
    'ba' => 'Bashkir',
    'be' => 'Belarusian(Беларуская)',
    'bg' => 'Bulgarian(Български)',
    'bh' => 'Bihari',
    'bi' => 'Bislama',
    'bm' => 'Bambara(Bamanankan)',
    'bn' => 'Bengali',
    'bo' => 'Tibetan',
    'br' => 'Breton',
    'bs' => 'Bosnian(Bosanski)',
    'ca' => 'Catalan(Català)',
    'ce' => 'Chechen',
    'ch' => 'Chamorro',
    'co' => 'Corsican',
    'cr' => 'Cree',
    'cs' => 'Czech(Čeština)',
    'cu' => 'Old Slavonic',
    'cv' => 'Chuvash',
    'cy' => 'Welsh(Cymraeg)',
    'da' => 'Danish(Dansk)',
    'de' => 'German(Deutsch)',
    'dv' => 'Maldivian',
    'dz' => 'Bhutani',
    'ee' => 'Ewe(Ɛʋɛ)',
    'el' => 'Greek(Ελληνικά)',
    'en' => 'English',
    'eo' => 'Esperanto',
    'es' => 'Spanish(Español)',
    'et' => 'Estonian(Eesti)',
    'eu' => 'Basque(Euskera)',
    'fa' => 'Persian(فارسی)',
    'ff' => 'Fulah(Fulfulde)',
    'fi' => 'Finnish(Suomi)',
    'fj' => 'Fiji',
    'fo' => 'Faeroese',
    'fr' => 'French(Français)',
    'fy' => 'Frisian(Frysk)',
    'ga' => 'Irish(Gaeilge)',
    'gd' => 'Scots Gaelic',
    'gl' => 'Galician(Galego)',
    'gn' => 'Guarani',
    'gu' => 'Gujarati',
    'gv' => 'Manx',
    'ha' => 'Hausa',
    'he' => 'Hebrew(עברית)',
    'hi' => 'Hindi(हिन्दी)',
    'ho' => 'Hiri Motu',
    'hr' => 'Croatian(Hrvatski)',
    'hu' => 'Hungarian(Magyar)',
    'hy' => 'Armenian(Հայերեն)',
    'hz' => 'Herero',
    'ia' => 'Interlingua',
    'id' => 'Indonesian(Bahasa Indonesia)',
    'ie' => 'Interlingue',
    'ig' => 'Igbo',
    'ik' => 'Inupiak',
    'is' => 'Icelandic(Íslenska)',
    'it' => 'Italian(Italiano)',
    'iu' => 'Inuktitut',
    'ja' => 'Japanese(日本語)',
    'jv' => 'Javanese',
    'ka' => 'Georgian',
    'kg' => 'Kongo',
    'ki' => 'Kikuyu',
    'kj' => 'Kwanyama',
    'kk' => 'Kazakh(Қазақ)',
    'kl' => 'Greenlandic',
    'km' => 'Cambodian',
    'kn' => 'Kannada(ಕನ್ನಡ)',
    'ko' => 'Korean(한국어)',
    'kr' => 'Kanuri',
    'ks' => 'Kashmiri',
    'ku' => 'Kurdish(Kurdî)',
    'kv' => 'Komi',
    'kw' => 'Cornish',
    'kg' => 'Kirghiz(Кыргыз)',
    'la' => 'Latin(Latina)',
    'lb' => 'Luxembourgish',
    'lg' => 'Luganda',
    'ln' => 'Lingala',
    'lo' => 'Laothian',
    'lt' => 'Lithuanian(Lietuvių)',
    'lv' => 'Latvian(Latviešu)',
    'mg' => 'Malagasy',
    'mh' => 'Marshallese',
    'mi' => 'Maori',
    'mk' => 'Macedonian(Македонски)',
    'ml' => 'Malayalam(മലയാളം)',
    'mn' => 'Mongolian',
    'mo' => 'Moldavian',
    'mr' => 'Marathi',
    'ms' => 'Malay(Bahasa Melayu)',
    'mt' => 'Maltese(Malti)',
    'my' => 'Burmese',
    'na' => 'Nauru',
    'nd' => 'North Ndebele',
    'ne' => 'Nepali',
    'ng' => 'Ndonga',
    'nl' => 'Dutch(Nederlands)',
    'nb' => 'Norwegian Bokmål(Bokmål)',
    'nn' => 'Norwegian Nynorsk(Nynorsk)',
    'nr' => 'South Ndebele',
    'nv' => 'Navajo',
    'ny' => 'Chichewa',
    'oc' => 'Occitan',
    'om' => 'Oromo',
    'or' => 'Oriya',
    'os' => 'Ossetian',
    'pa' => 'Punjabi',
    'pi' => 'Pali',
    'pl' => 'Polish(Polski)',
    'ps' => 'Pashto(پښتو)',
    'pt-pt' => 'Portuguese, Portugal(Português)',
    'pt-br' => 'Portuguese, Brazil(Português)',
    'qu' => 'Quechua',
    'rm' => 'Rhaeto-Romance',
    'rn' => 'Kirundi',
    'ro' => 'Romanian(Română)',
    'ru' => 'Russian(Русский)',
    'rw' => 'Kinyarwanda',
    'sa' => 'Sanskrit',
    'sc' => 'Sardinian',
    'sd' => 'Sindhi',
    'se' => 'Northern Sami',
    'sg' => 'Sango',
    'sh' => 'Serbo-Croatian',
    'si' => 'Sinhala(සිංහල)',
    'sk' => 'Slovak(Slovenčina)',
    'sl' => 'Slovenian(Slovenščina)',
    'sm' => 'Samoan',
    'sn' => 'Shona',
    'so' => 'Somali',
    'sq' => 'Albanian(Shqip)',
    'sr' => 'Serbian(Српски)',
    'ss' => 'Siswati',
    'st' => 'Sesotho',
    'su' => 'Sudanese',
    'sv' => 'Swedish(Svenska)',
    'sw' => 'Swahili(Kiswahili)',
    'ta' => 'Tamil(தமிழ்)',
    'te' => 'Telugu(తెలుగు)',
    'tg' => 'Tajik',
    'th' => 'Thai(ภาษาไทย)',
    'ti' => 'Tigrinya',
    'tk' => 'Turkmen',
    'tl' => 'Tagalog',
    'tn' => 'Setswana',
    'to' => 'Tonga',
    'tr' => 'Turkish(Türkçe)',
    'ts' => 'Tsonga',
    'tt' => 'Tatar(Tatarça)',
    'tw' => 'Twi',
    'ty' => 'Tahitian',
    'ug' => 'Uighur',
    'uk' => 'Ukrainian(Українська)',
    'ur' => 'Urdu(اردو)',
    'uz' => "Uzbek(o'zbek)",
    've' => 'Venda',
    'vi' => 'Vietnamese(Tiếng Việt)',
    'wo' => 'Wolof',
    'xh' => 'Xhosa(isiXhosa)',
    'yi' => 'Yiddish',
    'yo' => 'Yoruba(Yorùbá)',
    'za' => 'Zhuang',
    'zh-hans' => 'Chinese, Simplified(简体中文)',
    'zh-hant' => 'Chinese, Traditional(繁體中文)',
    'zu' => 'Zulu(isiZulu)',
  );
  return $lang ? $languages[$lang] : $languages;
}
