<?php
// $Id$

function _user_admin_tabs() {
  dd_set_tabs(array(
    l(t('user', '用户设置'), 'admin/users/settings'),
    l(t('user', '用户列表'), 'admin/users'),
    l(t('user', '新增用户'), 'admin/users/add'),
    l(t('user', '用户权限'), 'admin/users/perms'),
    l(t('user', '角色管理'), 'admin/users/roles'),
    l(t('user', '用户字段'), 'admin/users/fields'),
  ));
}

function user_admin_default() {
  _user_admin_tabs();

  $header = array(
    t('user', '帐号'),
    array('data' => t('user', '昵称'), 'field' => 'name'),
    array('data' => t('user', '登录次数'), 'field' => 'count'),
    array('data' => t('user', '在线时间'), 'field' => 'online'),
    array('data' => t('user', '注册时间'), 'field' => 'created', 'sort' => 'desc'),
    array('data' => t('user', '最近活动'), 'field' => 'active'),
    t('user', '操作')
  );

  $query = 'SELECT uid, name, mail, count, online, created, active, status FROM {users} WHERE uid > 0';
  
  if (!empty($_GET['keyword'])) {
    $keyword = '%' . urldecode($_GET['keyword']) . '%';
    $args = array($keyword, $keyword);
    $query .= ' AND (name LIKE ? OR mail LIKE ?)';
  } else {
    $args = NULL;
  }
  
  if ($fetch = pager_query($query. table_sql($header), 20, 0, NULL, $args)) {
    foreach ($fetch as $o) {
    	$items = array(l(t('user', '编辑'), 'admin/users/view/' . $o->uid . '/edit'));
    	
    	switch ($o->status) {
    		case 1:
    			$items[] = l(t('user', '阻止'), 'admin/users/view/' . $o->uid . '/hide', array('attributes' => array('class' => 'confirm')));
    		break;
    		case 2:
    			$items[] = l(t('user', '解冻'), 'admin/users/view/' . $o->uid . '/show', array('attributes' => array('class' => 'confirm')));
    		break;
    		default:
    			$items[] = l(t('user', '激活'), 'admin/users/view/' . $o->uid . '/active', array('attributes' => array('class' => 'confirm')));
    	}
      $items[] = l(t('user', '踢下线'), 'admin/users/view/' . $o->uid . '/logout', array(
        'attributes' => array('alt' => t('user', "如该用户处于登录状态，强行让其下线。\n确认吗？"), 'class' => 'confirm')
      ));
      $table[] = array(
        $o->mail, $o->name, $o->count, format_interval($o->online), format_date($o->created), format_date($o->active),
        dd_imp($items)
      );
    }
  }

  $output = dd_get_form('form_admin_search');
  $output .= theme('table', $header, $table, array('class' => 'center_table'));
  $output .= pager_view();
  return $output;
}

/**
 * 管理员编辑用户
 * @param object $ac
 *  用户对象
 */
function user_admin_edit_form($ac) {
  $form = user_edit_form($ac, 'admin');
  
  if (!empty($form['fields']['fields'])) {
    foreach ($form['fields']['fields'] as $key => $field) {
      unset($field['#required']);
      $form['fields']['fields'][$key] = $field;
    }
  }

  return $form;
}

// 用户资料
function user_admin_view($ac) {
	switch (arg(4)) {
		case 'edit':
		  _user_admin_tabs();
			dd_set_sub_tabs(array(
        l(t('user', '%string 的资料编辑', array('%string' => $ac->name)), 'admin/users/view/'.$ac->uid.'/edit'),
        l(t('user', '密码修改'), 'admin/users/view/'.$ac->uid.'/newpass')
			));
		return dd_get_form('user_admin_edit_form', $ac);
		case 'newpass':
      _user_admin_tabs();
      dd_set_sub_tabs(array(
        l(t('user', '%string 的资料编辑', array('%string' => $ac->name)), 'admin/users/view/'.$ac->uid.'/edit'),
        l(t('user', '密码修改'), $_GET['q'])
      ));
		return dd_get_form('user_admin_new_pass', $ac);
    case 'logout':
      // 强制用户下线
      global $conf;
      if (!empty($conf['dida_user_enforce_logout'])) {
        $conf['dida_user_enforce_logout'][] = $ac->uid;
      } else {
        $conf['dida_user_enforce_logout'] = array($ac->uid);
      }
      var_set('dida_user_enforce_logout', $conf['dida_user_enforce_logout']);
			dd_goto('admin/users');
		exit;     
		case 'hide':
			if (db_exec('UPDATE {users} SET status = 2, session = 0 WHERE uid = ?', array($ac->uid))) {
        /**
         * 改变用户状态
         * 触发 hook_user_status()
         */
        module_invoke_all('user_status', $ac->status, $ac);
      }
			dd_goto('admin/users');
		exit;
		case 'show': case 'active':
			if (db_exec('UPDATE {users} SET status = 1, session = 0 WHERE uid = ?', array($ac->uid))) {
        /**
         * 改变用户状态
         * 触发 hook_user_status()
         */
        module_invoke_all('user_status', $ac->status, $ac);
      }
			dd_goto('admin/users');
		exit;
	}
}

/**
 * 管理员为用户修改密码，修改后下次登录生效
 * @param (object) $ac
 */
function user_admin_new_pass($ac) {
  $form['fields']['name'] = array(
    '#type' => 'value',
    '#title' => t('user', '帐号'),
    '#value' => $ac->name
  );
  
  $form['fields']['mail'] = array(
    '#type' => 'value',
    '#title' => t('user', '邮箱'),
    '#value' => $ac->mail
  );
  
  $form['fields']['uid'] = array(
    '#type' => 'hidden',
    '#constant' => 1,
    '#default_value' => $ac->uid
  );
  
  $form['fields']['pass'] = array(
    '#type' => 'password',
    '#required' => 1,
    '#title' => t('user', '新密码'),
  );
  
  $form['fields']['pass2'] = array(
    '#type' => 'password',
    '#required' => 1,
    '#title' => t('user', '重复密码'),
  );
  
  $form['settings'] = array(
    '#validate' => array('user_admin_new_pass_validate')
  );
  
  $form['fields']['button']['#type'] = 'submit';
  return $form;
}

function user_admin_new_pass_validate($form, $v) {
  
  if ($v['pass'] != $v['pass2']) {
    $e['pass']['#error'][] = t('user', '两次密码不一致');
  } else if ($v['pass']) {
    $p = user_get_salt_pass($v['pass']);
    if (db_exec('UPDATE {users} SET pass = ?, salt = ? WHERE uid = ?', array($p['pass'], $p['salt'], $v['uid']))) {
      dd_set_message(t('user', '密码修改成功'));
    } else {
      dd_set_message(t('user', '密码修改失败'), 'error');
    }
  }
  
  return $e;
}

/**
 * 角色组权限分配
 */
function user_set_perm() {
  global $conf;
  _user_admin_tabs();
  $perms = _user_perm();
  if ($fetch = db_query('SELECT * FROM {role} ORDER BY rid DESC', NULL, array('fetch' => 'array'))) {
    foreach ($fetch as $role) {
      if ($role['perm']) {
        $role['perm'] = unserialize($role['perm']);
      } else {
        $role['perm'] = array();
      }
      $roles[$role['rid']] = $role;
    }
  }
  foreach ($conf['modules'] as $name => $info) {
    if ($perms[$name]) {
      foreach ($perms[$name] as $key => $value) {
        foreach ($roles as $rid => $role) {
          $form['fields']['perm'][$name][$key][$rid] = array(
            '#type' => 'checkbox',
            '#default_value' => $role['perm'][$name][$key] ? 1: 0,
            '#attributes' => array('title' => $value)
          );
        }
      }
    }
  }
  $form['settings'] = array(
    '#theme' => '_user_roles_theme',
    '#title' => t('user', '权限分配'),
    '#validate' => array('user_set_perm_validate'),
    '#data' => $roles
  );
  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('user', '确认操作'),
  );
  return $form;
}

function _user_roles_theme($form, $fields) {
  global $conf;
  $roles = array();
  $roles = $form['settings']['#data'];
  $count = count($roles) + 1;
  $header[] = t('user', '权限名称');
  foreach ($roles as $role) {
    $header[] = $role['name'];
  }
  foreach ($fields['perm'] as $name => $perm) {
    $table[] = array(array('data' => t($name, $conf['modules'][$name]['name']), 'colspan' => $count, 'class' => 'user_admin_role_module'));
    foreach ($perm as $value => $role) {
      $p = array();
      $p[] = $role[key($role)]['#attributes']['title'];
      foreach ($roles as $rid => $v) {
        $p[] = $role[$rid]['#html'];
      }
      $table[] = $p;
    }
  }
  $table[] = array(array('data' => $fields['submit']['#html'], 'colspan' => $count, 'align' => 'right'));
  $output .= theme('table', $header, $table, array('class' => 'table_fixed_header'));
  return $output;
}

function user_set_perm_validate($form, $value) {
  if ($value['perm']) {
    $i = 1;
    $role = $form['settings']['#data'];
    foreach ($value['perm'] as $name => $perm) {
      if ($perm) {
        foreach ($perm as $p => $rids) {
          foreach ($role as $rid => $v) {
            if ($value['perm'][$name][$p][$rid]) {
              $roles[$rid][$name][$p] = $i;
              ++$i;
            }
          }
        }
      }
    }
  }
  if ($roles) {
    foreach ($role as $rid => $v) {
      db_exec('UPDATE {role} SET perm = ? WHERE rid = ?', array($roles[$rid] ? serialize($roles[$rid]) : '', $rid));
    }
    var_del('roles_perm_cache');
  }
}

/**
 * 管理员新增会员
 */
function user_admin_add() {
  global $conf;

	_user_admin_tabs();
  $form = user_register_form('admin');
  unset($form['fields']['captcha']);

  $form['fields']['status'] = array(
    '#type' => 'radio',
    '#title' => t('user', '状态'),
    '#default_value' => var_get_key('user', 'status', 1) == 1 ? 1 : 0,
    '#required' => 1,
    '#description' => t('user', '激活状态才能登录'),
    '#options' => array(t('user', '隐藏'), t('user', '激活')),
  );
 
  $opt = user_roles_opt();
  unset($opt[var_get_key('user', 'anonymous_role', 1)]);

  $form['fields']['rid'] = array(
    '#type' => 'select',
    '#title' => t('user', '角色'),
    '#required' => 1,
    '#options' => $opt,
  );

  $form['fields']['button'] = array(
    '#type' => 'submit',
    '#weight' => 100,
    '#submit' => 'user_admin_add_submit',
    '#value' => t('user', '确认注册')
  );
  unset($form['settings']);
  $form['settings'] = array(
    '#validate' => array('user_register_validate'),
    '#ajax_validate' => true,
  );
  return $form;
}

function user_admin_add_submit($form, $value) {
  if ($account = user_insert((object)$value)) {
    dd_set_message(t('user', '用户 %string 添加成功', array('%string' => $account->mail)));
    $value['nid'] = $value['uid'] = $account->uid;
    field_get_node_save('user', 'field', (object)$value, NULL, $form);
    dd_goto('admin/users/add');
  } else {
    dd_set_message(t('user', '用户 %string 添加失败', array('%string' => $account->mail)));
  }
}

function user_set_settings() {
  _user_admin_tabs();

  $form['fields']['user']['register'] = array(
    '#type' => 'radio',
    '#title' => t('user', '允许注册'),
    '#required' => true,
    '#default_value' => var_get_key('user', 'register', 1),
    '#options' => array(t('user', '否'), t('user', '是')),
  );

  $form['fields']['user']['status'] = array(
    '#type' => 'radio',
    '#title' => t('user', '激活帐号'),
    '#required' => true,
    '#default_value' => var_get_key('user', 'status', 1),
    '#options' => array(t('user', '通过邮箱激活'), t('user', '不需要激活'), -1 => t('user', '手动激活')),
  );

  $form['fields']['user']['anonymous'] = array(
    '#type' => 'textfield',
    '#title' => t('user', '游客名称'),
    '#required' => true,
    '#default_value' => var_get_key('user', 'anonymous', t('user', '游客')),
  );

  $form['fields']['user']['select_theme'] = array(
    '#type' => 'radio',
    '#title' => t('user', '允许选择主题'),
    '#required' => true,
    '#default_value' => var_get_key('user', 'select_theme', 0),
    '#options' => array(t('user', '否'), t('user', '是')),
  );

  $form['fields']['user']['select_lang'] = array(
    '#type' => 'radio',
    '#title' => t('user', '允许选择语言'),
    '#required' => true,
    '#default_value' => var_get_key('user', 'select_lang', 0),
    '#options' => array(t('user', '否'), t('user', '是')),
  );

  $form['fields']['user']['redirect'] = array(
    '#type' => 'textfield',
    '#title' => t('user', '登录后跳转至'),
    '#required' => true,
    '#default_value' => var_get_key('user', 'redirect'),
    '#description' => t('user', '可使用 user、system 通配符，<a href="!url">通配符列表</a>', array('!url' => url('admin/token')))
  );

  $form['fields']['user']['default_avatar'] = array(
    '#type' => 'textfield',
    '#title' => t('user', '用户默认头像'),
    '#required' => 1,
    '#default_value' => var_get_key('user', 'default_avatar', 'misc/images/avatar.gif'),
  );

  $form['fields']['user']['default_role'] = array(
    '#type' => 'select',
    '#title' => t('user', '用户默认角色'),
    '#required' => 1,
    '#default_value' => var_get_key('user', 'default_role', 2),
    '#options' => user_roles_opt(),
    '#description' => t('user', '注册用户的默认角色权限'),
  );

  $form['fields']['user']['anonymous_role'] = array(
    '#type' => 'select',
    '#title' => t('user', '游客默认角色'),
    '#required' => 1,
    '#default_value' => var_get_key('user', 'anonymous_role', 1),
    '#options' => user_roles_opt(),
    '#description' => t('user', '游客的默认角色权限'),
  );

  $form['fields']['user']['custom_view'] = array(
    '#type' => 'select',
    '#title' => t('user', '用户主页'),
    '#required' => 1,
    '#default_value' => var_get_key('user', 'custom_view', 0),
    '#options' => array(t('user', '系统默认'), t('user', '自定义')),
    '#description' => t('user', '若选择自定义，用户主页将不会有任何显示，仅传递可用变量，请自行在 user.tpl.php 中布局'),
  );
 
  $form['fields']['user']['is_pass'] = array(
    '#type' => 'radio',
    '#title' => t('user', '验证密码'),
    '#required' => true,
    '#default_value' => var_get_key('user', 'is_pass', 1),
    '#options' => array(t('user', '否'), t('user', '是')),
    '#description' => t('user', '用户编辑个人资料时是否验证密码'),
  );
 
  $options = array(0 => t('user', '默认'));
  if ($modules = var_get('modules')) {
    foreach ($modules as $module => $info) {
      $options[$module] =  $info['name'];
    }
  }

  $form['fields']['user']['custom_view_module'] = array(
    '#type' => 'select',
    '#title' => t('user', '交给模块处理'),
    '#default_value' => var_get_key('user', 'custom_view_module'),
    '#options' => $options,
    '#description' => t('user', '将用户首页完全交给某个模块处理，除非某个模块建议这么做，否则请不要选择'),
  );
 
  $form['settings'] = array(
    '#validate' => array('user_set_settings_validate')
  );

  $form['fields']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('user', '确认设置'),
  );

  return $form;
}

function user_set_settings_validate($form, $v) {
  if ($v['user']['anonymous_role'] == $v['user']['default_role']) {
    dd_set_message(t('user', '游客默认角色不能与用户默认角色相同'), 'error');
    return true;
  }
  foreach ($v['user'] as $key => $value) {
    $val[$key] = $value;
  }
  var_set('user', $val);
  if ($v['user']['anonymous']) db_exec('UPDATE {users} SET name = ? WHERE uid = 0', array($v['user']['anonymous']));
}

function user_admin_roles() {
  _user_admin_tabs();
  dd_set_sub_tabs(array(l(t('user', '角色'), 'admin/users/roles'), l(t('user', '新增'), 'admin/users/roles/add')), NULL, 'ul', array('class' => 'admin_tabs'));
  
  if (arg(3) != 'add') {
    switch ($_GET['op']) {
      case 'edit':
        if ($_POST['value']) {
          db_exec('UPDATE {role} SET name = ? WHERE rid = ?', array(trim(check_plain($_POST['value'])), $_GET['id']));
          var_del('user_roles');
          echo $_POST['value'];
        }
      exit;
      case 'delete':
        if (db_exec('DELETE FROM {role} WHERE rid = ?', array($_GET['rid']))) {
          var_del('user_roles');
          echo 1;
        }
      exit;
    }
    if ($fetch = pager_query('SELECT * FROM {role} ORDER BY rid ASC', 20)) {
      foreach ($fetch as $o) {
        $perm = '';
        if ($o->perm) {
          $perm = _user_role_perms(unserialize($o->perm));
        }
        $table[] = array(
          array('data' => '<div class="jqedit_text admin_role" ajaxurl="?op=edit&id='.$o->rid.'">' . $o->name . '</div>'),
          $perm, $o->count,
          dd_imp(array(
            l(t('user', '权限'), 'admin/users/perms'),
            '<a href="?op=delete&rid='.$o->rid.'" class="confirmajax">'.t('user', '删除').'</a>'
          ))
        );
      }
    }
    
    $header = array(
      array('data' => t('user', '角色名称'), 'class' => 'user_admin_roles_name'),
      array('data' => t('user', '权限'), 'class' => 'user_admin_roles_perms'),
      array('data' => t('user', '总人数'), 'class' => 'user_admin_roles_count'),
      array('data' => t('user', '操作'), 'class' => 'user_admin_roles_op')
    );

    $output .= theme('table', $header, $table, array('id' => 'user_admin_roles_table'));
    $output .= pager_view();
    
    dd_jqedit(array('dom' => '.jqedit_text', 'limit' => 16, 'opt' => array('type' => 'charcounter', 'onblur' => 'ignore', 'width' => 80)));
  } else {
    $output .= dd_get_form('user_admin_roles_form');
  }
  return $output;
}

function user_admin_roles_form() {
  $form['settings']['#validate'][] = 'user_admin_roles_form_validate';
  $form['fields']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('user', '角色名称'),
    '#required' => 1,
    '#description' => t('user', '16 个字符以内'),
  );
  $form['fields']['submit']['#type'] = 'submit';
  return $form;
}

function user_admin_roles_form_validate($form, $v) {
  if (mb_strlen($v['name']) > 16) {
    $t['name']['#error'][] = t('user', '名称过长');
  } else if (db_exec('INSERT INTO {role} (name) VALUES (?)', array(trim($v['name'])))) {
    dd_set_message(t('user', '角色 %string 添加成功', array('%string' => $v['name'])));
    var_del('user_roles');
    dd_goto('admin/users/roles');
  } else {
    dd_set_message(t('user', '添加失败，角色已经存在'));
  }
  return $t;
}

/**
 * 用户字段管理
 */
function user_admin_fields() {
	$tabs = array(
		l(t('user', '字段管理'), 'admin/users/fields'),
		l(t('user', '添加字段'), 'admin/users/fields/add'),
		l(t('user', '分组管理'), 'admin/users/fields/groups'),
		l(t('user', '导出字段'), 'admin/users/fields/export')
	);

  if (!field_get_cache_field_key('user', 'field', 'avatar')) {
    dd_set_help(t('user', '你需要创建一个头像字段吗？!link',
    array('!link' => l(t('user', '点这儿'), 'admin/users/fields/test'))));
  }
	
  $args = arg();
  switch ($args[3]) {
  	case 'add':
      $field->type = 'field';
      $field->module = 'user';
      $field->redirect = url('admin/users/fields');
  		$output = dd_get_form('field_form', $field);
  	break;
  	case 'export':
  	  $output = field_export(0, 'user', 'field');
  	break;
    case 'groups':
      $output = dd_get_form('field_groups', 'user', 'field');
    break;
  	case 'test':
  	  _user_field_test();
  	break;
  	default:
  		$output = field_lists('user', 'field');
  }
  
  dd_set_tabs($tabs);
  return $output;
}

/**
 * 创建一个头像字段
 */
function _user_field_test() {
  if (!field_get_cache_field_key('user', 'field', 'avatar')) {
    $field = array(
      'name' => t('field', '头像'),
      'module' => 'user',
      'type' => 'field',
      'field_type' => 'file',
      'field_key' => 'avatar',
      'weight' => -20,
      'data' => array(
        'multi' => 1,
        'filesize' => 4096,
        'filename' => '[uid]',
        'extension' => 'jpg',
        'savepath' => 'avatar',
        'thumbs' => array(
          'thumb' => '85x85',
          'medium' => '200x200',
        )
      )
    );
    
    $data = (object) $field;
    field_save($data);
  } else {
    dd_set_message(t('user', '已经创建了'));
  }
  
  dd_goto('admin/users/fields');
}
