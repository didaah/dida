<?php
// $Id$

function block_admin_default() {
  dd_goto('admin/blocks/settings/' .$GLOBALS['conf']['default_theme']);
}

function block_admin_form($theme) {
	global $conf;
	if ($conf['themes'][$theme]) {
		foreach ($conf['themes'] as $links) {
			$item[] = l($links['name'], 'admin/blocks/settings/'.$links['filename']);
		}
    $item[] = l(t('block', '缓存管理'), 'admin/blocks/cache/'.$theme);
    $item[] = l(t('block', '添加区块'), 'admin/blocks/boxes/add');
    dd_set_tabs($item);
		
    if ($blocks = block_cache_lists($theme)) {
      $regions = $conf['themes'][$theme]['region'];
      $regions[0] = t('block', '无');
      foreach ($blocks as $block) {
        if ($regions[$block->region]) {
          $region = $block->region;
          $defaultValue = $block->region;
        } else {
          $defaultValue = $region = 0;
        }
        
        $form['fields']['region'][$region][$block->bid]['region'] = array(
          '#title' => t('block', '区域'),
          '#type' => 'select',
          '#default_value' => $defaultValue,
          '#prefix' => '<div class="block_item"><div class="title">' . $block->title . '</div>',
          '#options' => $regions,
        );
        $form['fields']['region'][$region][$block->bid]['weight'] = array(
          '#title' => t('block', '权重'),
          '#type' => 'weight',
          '#default_value' => $block->weight,
          '#suffix' => '',
          '#start' => -30,
          '#end' => 30
        );
        $form['fields']['region'][$region][$block->bid]['visibility'] = array(
          '#title' => t('block', '启用'),
          '#type' => 'checkbox',
          '#default_value' => $block->visibility,
          '#suffix' => '</div>',
          '#field_suffix' => theme('item_list', array(l(t('block', '配置'), $_GET['q'].'/'.$block->bid), l(t('block', '恢复默认值'), $_GET['q'].'/'.$block->bid.'/default', array('attributes' => array('class' => 'confirm', 'alt' => t('block', '此操作不可恢复。将删除针对此区块的自定义配置，恢复初始值！')))))),
        );
      }
      
      $form['settings']['#validate'] = array('block_admin_form_validate');
      $form['fields']['submit']['#type'] = 'submit';
      
      dd_set_title(array($conf['themes'][$theme]['name'], t('block', '区块管理')));
    } else {
      dd_set_message(t('block', '这个主题不能设置区块'), 'error');
    }
    dd_add_css('modules/block/block.admin.css');
		return $form;
	} else {
		dd_get_not();
	}
}

function block_admin_form_validate($form, $v) {
  global $conf;
  if ($v['region']) {
    foreach ($v['region'] as $region => $blocks) {
      foreach ($blocks as $bid => $block) {
        db_exec('UPDATE {blocks} SET region = ?, weight = ?, visibility = ? WHERE bid = ?', array($block['region'], $block['weight'], ($block['visibility'] ? $block['visibility'] : 0), $bid));
      }
    }
    cache_del('cid', 'theme_'.arg(3).'_list', 'cache_block');
    var_del('theme_' . arg(3) . '_view');
    var_del('theme_' . arg(3) . '_boxes_view');
    dd_set_message(t('block', '保存成功'));
  }
}

// 区块配置
function block_settings_form($theme, $block) {
	$blocks = call_user_func($block->module.'_block_list', $theme, $block);
	
	if ($config = $blocks[$block->delta]) {
		if (arg(5) == 'default') {
    	$config['settings']['custom'] = array(
    		'custom' => $config->custom
    	);
      
      if (empty($config['region'])) $config['region'] = '';

      if (block_update_info($block->bid, $config)) {
      	var_del('theme_' . $theme . '_view');
        var_del('theme_' . $theme . '_boxes_view');
      	dd_set_message(t('block', '已恢复默认值'));
        dd_goto('admin/blocks/settings/'.$theme);
      }
		}
		dd_set_title(array($config['title'], t('block', '区块配置'), t('block', '区块管理')));
		
	  $item = array(
	    l($config['title'], $_GET['q']),
	    l(t('block', '区块列表'), 'admin/blocks/settings/'.$theme)
	  );
	  
		dd_set_tabs($item);
    
		$form['fields']['module']= array(
			'#title' => t('block', '所属模块'),
			'#weight' => -99,
			'#value' => $block->module
		);
	  $form['fields']['title'] = array(
	    '#title' => t('block', '显示标题'),
	    '#type' => 'textfield',
	    '#default_value' => $block->title,
	    '#description' => t('block', '隐藏标题则留空'),
	  );
		
	  $form['fields']['settings']['display']['type'] = array(
	    '#type' => 'radio',
	    '#fieldset_prefix' => 'asc',
	    '#fieldset_legend' => t('block', '显示设置'),
	    '#default_value' => $block->settings['display'] ? $block->settings['display']['type'] : NULL,
	    '#options' => array(
	      1 => t('block', '在下列页面显示'),
	      2 => t('block', '除下列页面外的其它页面显示'),
	    )
	  );

    $description = array(
      t('block', '根据系统路径设置区块显示，可使用以下三种匹配模式：'),
      t('block', '普通模式：一行一个路径。斜杠(/)代表整个路径。如：user 匹配单个页面，而 user/ 匹配以 user 下的所有页面。&lt;front&gt; 代表首页'),
      t('block', '正则模式：必须以斜杠(/)开始和结束。如：/user\/.*?\/edit/ 将匹配 user/*/edit'),
    );

    if ($GLOBALS['_filter_is_php_code'] == true && user_access('filter use php')) {
      $form['fields']['settings']['display']['type']['#options'][3] = t('block', '当 PHP 代码返回 TRUE 时显示');
      $description[] = t('block', 'PHP模式：如：if ($GLOBALS[\'user\']->uid == 1) return true;不能包含 PHP 标签。必须用 return 而不是 echo');
    }

	  $form['fields']['settings']['display']['body'] = array(
	    '#type' => 'textarea',
	    '#fieldset_suffix' => 1,
	    '#default_value' =>$block->settings['display'] ? $block->settings['display']['body'] : NULL,
	    '#attributes' => array('rows' => 6, 'cols' => 60),
	    '#description' => theme('item_list', $description)
    );

    $function = $block->module.'_block_setting';
    if (function_exists($function) && $custom = $function($theme, $block)) {
			if (is_array($custom)) {
				$form['fields']['settings']['custom'] = $custom;
			}
		}
	  $form['fields']['bid'] = array(
	    '#type' => 'hidden',
	    '#default_value' => $block->bid,
	    '#constant' => true,
	  );
	  $form['fields']['submit']['#type'] = 'submit';
	  $form['settings'] = array(
	  	'#validate' => array('block_settings_form_validate'),
	  	'#data' => $block
	  );
  } else {
  	dd_set_message(t('block', '数据已过期，模块(%string)中已不存在这个区块，请更新 <a href="!url">区块列表缓存</a>', array('%string' => $block->module, '!url' => url('admin/blocks/cache/'.$theme))), 'error');
  }
  return $form;
}

function block_settings_form_validate($form, $v) {
	$block = $form['settings']['#data'];
	$theme = arg(3);
	if (mb_strlen($v['title']) > 60) {
		$t['title']['#error'][] = t('block', '标题长度不超过 %length 字符', array('%length' => 60));
	} else {
    $function = $block->module.'_block_save';
    if (function_exists($function)) {
      $t = $function($theme, $block, $v);
    }
  }
  if (!$t) {
		if ($v['settings']['display']['body']) {
			$v['settings']['display']['body'] = trim($v['settings']['display']['body']);
		}
	  db_exec('UPDATE {blocks} SET title = ?, settings = ? WHERE bid = ?', array(trim($v['title']), serialize($v['settings']), $v['bid']));
	  var_del('theme_' . $theme . '_view');
    var_del('theme_' . $theme . '_boxes_view');
	  dd_set_message(t('block', '设置成功'));
    dd_goto_redirect('admin/blocks/settings/'.$theme);
  }
  return $t;
}

function block_admin_cache($theme) {
  $item = array(
    l(t('block', '缓存管理'), $_GET['q']),
    l(t('block', '区块列表'), 'admin/blocks/settings/'.$theme)
  );
  
	dd_set_tabs($item);
  
  $form['fields']['cache'] = array(
    '#type' => 'checkbox',
    '#options' => array(
      'list' => t('block', '清除区块列表缓存'),
      'view' => t('block', '清除区块内容缓存'),
    )
  );
  $form['settings']['#validate'] = array('block_admin_cache_validate');
  $form['fields']['submit']['#type'] = 'submit';
  return $form;
}

function block_admin_cache_validate($form, $v) {
  if ($v['cache']) {
    global $conf;
    $theme = arg(3);
    foreach ($v['cache'] as $key) {
      switch ($key) {
        case 'list':
          block_cache_lists($theme);
        break;
        case 'view':
          var_del('theme_' . $theme . '_view');
          var_del('theme_' . $theme . '_boxes_view');
      }
    }
    dd_set_message(t('block', '清除成功'));
  }
}
