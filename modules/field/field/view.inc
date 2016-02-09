<?php
// $Id$

/**
 * @file field 字段显示
 * 触发 hook_TYPE_field_node_view() 前，将执行 module_include_file('field/view.inc');
 * 即，如果模块想实现以上 hook，可将代码放在 .module 文件中，也可以放在模块目录下的 field/view.inc 文件中
 * 其它模块若要触发以上 hook，必须先执行 module_include_file('field/view.inc')
 */

/**
 * radio
 * @return array $content
 *  返回的内容必须包含 #content，该值将直接输出
 */
function field_radio_field_node_view($field, $node, $view_type) {

  // 约定返回的参数
  $content = array(
    '#name' => $field->name,
    '#content' => l($node->fields[$field->field_key]->name, 'category/' . $node->fields[$field->field_key]->tid),
    '#field_type' => 'category', // radio select selects chcekbox 统一视做分类
    '#field_key' => $field->field_key
  );

  return $content;
}

function field_select_field_node_view($field, $node, $view_type) {

  $content = array(
    '#name' => $field->name,
    '#content' => l($node->fields[$field->field_key]->name, 'category/' . $node->fields[$field->field_key]->tid),
    '#field_type' => 'category',
    '#field_key' => $field->field_key
  );

  return $content;
}
 
function field_selects_field_node_view($field, $node, $view_type) {
  $data = '';

  foreach ($node->fields[$field->field_key] as $term) {
    $data .= l($term->name, 'category/' . $term->tid);
  }

  $content = array(
    '#name' => $field->name,
    '#content' => $data, 
    '#field_type' => 'category',
    '#field_key' => $field->field_key
  );

  return $content;
}
 
function field_checkbox_field_node_view($field, $node, $view_type) {
  $data = '';

  foreach ($node->fields[$field->field_key] as $term) {
    $data .= l($term->name, 'category/' . $term->tid);
  }

  $content = array(
    '#name' => $field->name,
    '#content' => $data, 
    '#field_type' => 'category',
    '#field_key' => $field->field_key
  );

  return $content;
}
  
function field_tags_field_node_view($field, $node, $view_type) {
  $data = '';

  foreach ($node->fields[$field->field_key] as $term) {
    $data .= l($term->name, 'category/' . $term->tid);
  }

  $content = array(
    '#name' => $field->name,
    '#content' => $data, 
    '#field_type' => 'category',
    '#field_key' => $field->field_key
  );

  return $content;
}

function field_textfield_field_node_view($field, $node, $view_type) {
  $data = check_plain($node->fields[$field->field_key]->text);

  if (!empty($field->data['validate']['type'])) {
  }

  $content = array(
    '#name' => $field->name,
    '#content' => $data, 
    '#field_type' => 'textfield',
    '#field_key' => $field->field_key
  );

  return $content;
}

function field_textarea_field_node_view($field, $node, $view_type) {
  $content = array(
    '#name' => $field->name,
    '#content' => filter_view($node->fields[$field->field_key]->text, $field->data['format']),
    '#field_type' => 'textarea',
    '#field_key' => $field->field_key
  );

  return $content;
}

function field_file_field_node_view($field, $node, $view_type) {
  $content = array(
    '#name' => $field->name,
    '#content' => '', 
    '#field_type' => 'file',
    '#field_key' => $field->field_key
  );

  foreach ($node->fields[$field->field_key] as $file) {
    // 将节点标题附加到图片名称
    if (!empty($node->title)) {
      $title = $node->title;
    } else if (!empty($node->name)) {
      $title = $node->name;
    }
    $title .= ' ' . $file->filename;

    if (in_array($file->filemime, array('image/jpg', 'image/png', 'image/gif', 'image/jpeg'))) {
      $content['#field_type'] = 'file_image'; // 文件中的图片类型
      if (!empty($field->data['thumbs']['medium'])) {
        $content['#content'] .= l(img(image_get_thumb($file->fid, $file->filepath, $field->data['thumbs']['medium'])), 'files/' . $file->fid . '/view',
        array('html' => 1, 'attributes' => array('target' => '_blank', 'title' => $title, 'alt' => $title)));
      } else {
        $content['#content'] .= l(img($file->filepath), 'files/' . $file->fid . '/view',
        array('html' => 1, 'attributes' => array('target' => '_blank', 'title' => $title, 'alt' => $title)));
      }
    } else {
      $content['#content'] .= l(($file->filename ? $file->filename : dirname($file->filepath)), 'files/' . $file->fid . '/view',
      array('attributes' => array('target' => '_blank', 'title' => $title, 'alt' => $title)));
    }
  }

  return $content;
}
