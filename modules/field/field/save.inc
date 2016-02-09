<?php
// $Id$

/**
 * @file field 保存提交表单中的字段数据
 * 触发 hook_TYPE_field_node_save() 前，将执行 module_include_file('field/save.inc');
 * 即，如果模块想实现以上 hook，可将代码放在 .module 文件中，也可以放在模块目录下的 field/save.inc 文件中
 * 其它模块若要触发以上 hook，必须先执行 module_include_file('field/save.inc')
 */

/**
 * radio
 */
function field_radio_field_node_save($field, $value, $node, &$form, $ac) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid 
  IN (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($value->nid, $field->field_id));

  if (!empty($value->fields[$field->field_key])) {
    db_exec('INSERT INTO {fields_term_node} (tid, nid) VALUES (?, ?)', array($value->fields[$field->field_key], $value->nid));
  }
}

/**
 * select
 */
function field_select_field_node_save($field, $value, $node, &$form, $ac) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid 
  IN (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($value->nid, $field->field_id));

  if (!empty($value->fields[$field->field_key])) {
    db_exec('INSERT INTO {fields_term_node} (tid, nid) VALUES (?, ?)', array($value->fields[$field->field_key], $value->nid));
  }
}

/**
 * selects
 */
function field_selects_field_node_save($field, $value, $node, &$form, $ac) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid 
  IN (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($value->nid, $field->field_id));

  if (!empty($value->fields[$field->field_key]) && is_array($value->fields[$field->field_key])) { 
    $tids = array();
 		foreach ($value->fields[$field->field_key] as $tid) {
			$tids[] = '(' . $tid .','.$value->nid . ')';
		}
		db_exec('INSERT INTO {fields_term_node} (tid, nid) VALUES '. implode(',', $tids));   
  }
}

/**
 * checkbox
 */
function field_checkbox_field_node_save($field, $value, $node, &$form, $ac) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid 
  IN (SELECT tid FROM {fields_term} WHERE field_id = ?)', array($value->nid, $field->field_id));

  if (!empty($value->fields[$field->field_key]) && is_array($value->fields[$field->field_key])) { 
    $tids = array();
 		foreach ($value->fields[$field->field_key] as $tid) {
			$tids[] = '(' . $tid . ',' . $value->nid . ')';
		}
		db_exec('INSERT INTO {fields_term_node} (tid, nid) VALUES '. implode(',', $tids));   
  }
}

/**
 * tags
 */
function field_tags_field_node_save($field, $value, $node, &$form, $ac) {
  db_exec('DELETE FROM {fields_term_node} WHERE nid = ? AND tid 
  IN (SELECT tid FROM {fields_term} WHERE field_id = ?)',  array($value->nid, $field->field_id));

  if (!empty($value->fields[$field->field_key]) && $tags = field_get_string_tags($value->fields[$field->field_key])) {
    $args = array();
    foreach ($tags as $tag) {
      if (!empty($node->fields[$field->field_key])) {
        if ($tid = array_search($tag, $node->fields[$field->field_key])) {
          $args[] = '(' . $tid . ',' . $value->nid . ')';
          continue;
        }
      }

      if ($tid = db_query('SELECT tid FROM {fields_term} WHERE name = ? AND field_id = ?',
      array($tag, $field->field_id), array('return' => 'column'))) {
        $args[] = '(' . $tid . ',' . $value->nid . ')';
      } else {
        $strlen = mb_strlen($tag);
        if (!empty($field->data['validate']['maxsize']) && $strlen > $field->data['validate']['maxsize']) {
          continue;
        }
        if (!empty($field->data['validate']['minsize']) && $strlen < $field->data['validate']['minsize']) {
          continue;
        }
        if (db_exec('INSERT INTO {fields_term} (field_id, name, pid, weight) VALUES (?, ?, 0, 0)', array($field->field_id, $tag))) {
          $args[] = '(' . db_last_insert_id() . ',' . $value->nid . ')';
        }
      }
    }
    db_exec('INSERT INTO {fields_term_node} (tid, nid) VALUES '. implode(',', $args));   
  }
}

/**
 * textfield 
 */
function field_textfield_field_node_save($field, $value, $node, &$form, $ac) {
  db_exec('DELETE FROM {fields_small} WHERE nid = ? AND field_id = ?', array($value->nid, $field->field_id));

  if (!empty($field->data['validate']['type'])) {
    if ($text = module_invoke_all('field_type_textfield', 'node_save', $field->data['validate']['type'], $field, $value->fields[$field->field_key])) {
      $value->fields[$field->field_key] = $text;
    }
  }

  if (isset($value->fields[$field->field_key])) {
    db_exec('INSERT INTO {fields_small} (nid, field_id, text) VALUES (?, ?, ?)',
    array($value->nid, $field->field_id, $value->fields[$field->field_key]));
  }
}

/**
 * textarea 
 */
function field_textarea_field_node_save($field, $value, $node, &$form, $ac) {
  db_exec('DELETE FROM {fields_long} WHERE nid = ? AND field_id = ?', array($value->nid, $field->field_id));
  if (isset($value->fields[$field->field_key])) {
    db_exec('INSERT INTO {fields_long} (nid, field_id, text) VALUES (?, ?, ?)',
    array($value->nid, $field->field_id, $value->fields[$field->field_key]));
  }
}

/**
 * file
 */
function field_file_field_node_save($field, $value, $node, &$form, $ac) {
  if (!empty($field->data['swfupload']) && module_exists('editor')) {
    // 将通过 swfupload 上传的文件关联到节点字段
    if (!empty($value->fields['_' . $field->module . $field->type . $field->field_key])) {
      db_exec('UPDATE {files} SET ext_id = ?, ext_id2 = ?, ext_type = ?, status = 1 WHERE ext_type = ?',
      array($field->field_id, $value->nid, 'field_file', $value->fields['_' . $field->module . $field->type . $field->field_key]));
    }
  }

  // 处理通过传统表单方式上传的文件
  if (!empty($_FILES['fields']) && $_FILES['fields']['name'][$field->field_key]) {
    $validate = _field_get_file_validate($field, $value);

    if (is_array($_FILES['fields']['name'][$field->field_key])) {
      // 上传多个文件
      foreach ($_FILES['fields']['name'][$field->field_key] as $i => $name) {
        if ($i > $field->data['multi']) break;
        if (!$message = file_validate_error($_FILES['fields']['error'][$field->field_key][$i])) {
          $file = array(
            'name' => $name,
            'type' => $_FILES['fields']['type'][$field->field_key][$i],
            'size' => $_FILES['fields']['size'][$field->field_key][$i],
            'tmp_name' => $_FILES['fields']['tmp_name'][$field->field_key][$i],
          );
          if (!file_save_upload($file, $validate)) {
            dd_set_message(t('field', '!name 上传失败', array('!name' => $file['name'])), 'error');
          }
        } else if ($message !== 4) {
          dd_set_message($message, 'error');
        }
      }
      if ($node->fields[$field->field_key]) {
        $k = $files + count($node->fields[$data->field_key]) - $field->data['multi'];
        if ($k > 0) {
          foreach ($node->fields[$data->field_key] as $file) {
            if ($k == 0) break;
            file_delete($file->fid, 1, $file->filepath);
            --$k;
          }
        }
      }
    } else {
      if (!$message = file_validate_error($_FILES['fields']['error'][$field->field_key])) {
        $file = array(
          'name' => $_FILES['fields']['name'][$field->field_key],
          'type' => $_FILES['fields']['type'][$field->field_key],
          'size' => $_FILES['fields']['size'][$field->field_key],
          'tmp_name' => $_FILES['fields']['tmp_name'][$field->field_key],
        );
        if (!file_save_upload($file, $validate)) {
          dd_set_message(t('field', '!name 上传失败', array('!name' => $file['name'])), 'error');
        }
      } else if ($message !== 4) {
        dd_set_message($message, 'error');
      }
    }
  }

  // 总共允许上传的文件数量，将多余的文件删除
  $count = !empty($field->data['multi']) ? $field->data['multi'] : 1;
  if ($fetch = db_query('SELECT fid, filepath FROM {files} WHERE ext_type = ? AND ext_id = ? AND ext_id2 = ? 
  ORDER BY fid DESC', array('field_file', $field->field_id, $value->nid))) {
    foreach ($fetch as $file) {
      if ($count < 1) {
        file_delete($file->fid, 1, $file->filepath);
      } else if (isset($value->fields['__field_file_edit_' . $file->fid . '_body'])) {
        // 修改文件标题和描述
        db_exec('UPDATE {files} SET filename = ?, filebody = ? WHERE fid = ?', array(
          $value->fields['__field_file_edit_' . $file->fid . '_name'],
          $value->fields['__field_file_edit_' . $file->fid . '_body'], $file->fid));
      }
      $count--;
    }
  }
}
