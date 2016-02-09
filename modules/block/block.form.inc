<?php
// $Id$

/**
 * 添加区块
 */
function block_add_form($block = NULL) {
	global $conf;
  $item[] = l(t('block', '管理区块'), 'admin/blocks/settings');
  $item[] = l(t('block', '编辑区块'), 'admin/blocks/boxes');
  $item[] = l(t('block', '添加区块'), 'admin/blocks/boxes/add');
  if ($block->bid) {
    $item[] = l(t('block', '编辑区块'), $_GET['q']);
  }
  dd_set_tabs($item);
	$form['settings'] = array(
		'#validate' => array('block_add_form_validate')
	);
	$form['fields']['title'] = array(
		'#title' => t('block', '标题'),
		'#type' => 'textfield',
		'#default_value' => $block->title,
		'#required' => 1,
	);
	$form['fields']['body'] = array(
    '#type' => 'textarea',
    '#title' => t('block', '内容'),
    '#required' => 1,
    '#default_value' => $block->body,
	);
	$form['fields']['bid'] = array(
    '#type' => 'hidden',
    '#default_value' => $block->bid,
    '#constant' => 1,
	);
	
  $form['fields']['format'] = filter_form_field($block->format);
  $form['fields']['format']['#options'][0] = t('block', '不使用以上格式，即内容不做任何处理');
  
  if (isset($block->format) && !$block->format) {
    $form['fields']['format']['#default_value'] = 0;
  }
  
	$form['fields']['submit']['#type'] = 'submit';
	return $form;
}

function block_add_form_validate($form, $v) {
  if (mb_strlen($v['title']) > 60) {
    $t['title']['#error'][] = t('block', '长度不超过 %length 字符', array('%length' => '60'));
    return $t;
  } else if ($v['bid']) {
    // 编辑
    $ok = db_exec('UPDATE {boxes} SET title = ?, body = ?, format = ? WHERE bid = ?', array($v['title'], $v['body'], $v['format'], $v['bid']), array('return' => 1));
  } else {
    // 新增
    $ok = db_exec('INSERT INTO {boxes} (title, body, format) VALUES (?, ?, ?)', array($v['title'], $v['body'], $v['format']));
  }
  if ($ok) {
    dd_set_message(t('block', '操作成功'));
  } else {
    dd_set_message(t('block', '操作失败'));
  }
  dd_goto('admin/blocks/boxes');
}

/**
 * 自定义区块列表
 */
function block_admin_boxes() {
  dd_set_tabs(array(l(t('block', '管理区块'), 'admin/blocks/settings'), l(t('block', '编辑区块'), 'admin/blocks/boxes'), l(t('block', '添加区块'), 'admin/blocks/boxes/add')));
  if ($_GET['op'] == 'delete' && $_GET['id']) {
    if (db_exec('DELETE FROM {boxes} WHERE bid = ?', array($_GET['id']))) {
      db_exec('DELETE FROM {blocks} WHERE module = ? AND delta = ?', array('block', 'boxes_'.$_GET['id']));
      dd_set_message(t('block', '删除成功'));
    } else {
      dd_set_message(t('block', '删除失败'));
    }
    dd_goto('admin/blocks/boxes');
  }
  if ($fetch = db_query('SELECT * FROM {boxes}')) {
	  foreach ($fetch as $o) {
	    $table[] = array(
	      t('block', $o->title),
	      dd_imp(
	        array(
	          l(t('block', '编辑'), 'admin/blocks/boxes/edit/'.$o->bid),
	          l(t('block', '删除'), 'admin/blocks/boxes', array('query' => 'op=delete&id='.$o->bid, 'attributes' => array('class' => 'confirm'))),
	        )
	      )
	    );
	  }
  }
  return theme('table', array(t('block', '名称'), t('block', '操作')), $table);
}