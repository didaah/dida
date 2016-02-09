<?php
// $Id$

/**
 * @file 节点中字段表单扩展
 * 触发 hook_TYPE_field_node_form() 前，将执行 module_include_file('field/form.inc');
 * 即，如果模块想实现以上 hook，可将代码放在 .module 文件中，也可以放在模块目录下的 field/form.inc 文件中
 * 其它模块若要触发以上 hook，必须先执行 module_include_file('field/form.inc')
 */

/**
 * radio
 */
function field_radio_field_node_form($field, $node, &$form, $ac) {
  $options = field_get_cache_term($field->module, $field->type);
  
  $form['fields']['fields'][$field->field_key] = array(
    '#title' => t(NULL, $field->name),
    '#type' => $field->field_type,
    '#default_value' => !empty($node->fields[$field->field_key]) ? $node->fields[$field->field_key]->tid : '',
    '#required' => $field->required,
    '#groups' => $field->groups,
    '#options' => field_get_term_options($options[$field->field_id], $field->required),
    '#description' => t(NULL, $field->data['input_description'])
  );

}

/**
 * checkbox
 */
function field_checkbox_field_node_form($field, $node, &$form, $ac) {
  $options = field_get_cache_term($field->module, $field->type);

  $field_value = array();

  if ($node->fields[$field->field_key]) {
    foreach ($node->fields[$field->field_key] as $o) {
      $field_value[$o->tid] = $o->tid;
    }
  }

  $form['fields']['fields'][$field->field_key] = array(
    '#title' => t(NULL, $field->name),
    '#type' => $field->field_type,
    '#default_value' => $field_value,
    '#groups' => $field->groups,
    '#required' => $field->required,
    '#options' => field_get_term_options($options[$field->field_id], 1),
    '#description' => t(NULL, $field->data['input_description'])
  );
}

/**
 * select
 */
function field_select_field_node_form($field, $node, &$form, $ac) {
  global $conf;

  $options = field_get_cache_term($field->module, $field->type);

  if (!empty($conf['field_term_children']) && !empty($conf['field_term_children'][$field->field_id])) {
    dd_add_js(array('fields' => array(
      $field->field_key => array(
        'url' => url('ajax', array('query' => 'fid=' . $field->field_id . '&op=select&module=field')),
      )
    )), 'setting');

    $form['fields']['fields']['field_root_' . $field->field_key] = array(
      '#title' => t(NULL, $field->name),
      '#type' => 'select',
      '#required' => $field->required,
      '#groups' => $field->groups,
      '#attributes' => array(
        'class' => 'field_option_children field_option_children_' . $field->field_key,
        'alt' => $field->field_key
      ),
      '#options' => field_get_term_options($options[$field->field_id]),
      '#description' => t(NULL, $field->data['input_description'])
    );

    if (!empty($_POST)  && isset($_POST['fields'][$field->field_key])) {
      $default_value = $_POST['fields'][$field->field_key];
    } else if ($node->fields[$field->field_key]) {
      $default_value = $node->fields[$field->field_key]->tid;
    } else {
      $default_value = 0;
    }
    
    if (!empty($default_value) && $terms = _field_get_form_term_childrens($default_value)) {
      static $ajaxId;
      $ajaxId = 0;
      $terms = array_reverse($terms);
      $form['fields']['fields']['field_root_' . $field->field_key]['#prefix'] = '<div class="form_item form_item_select" id="form_field_root_' . $field->field_key . '">';
      $form['fields']['fields']['field_root_' . $field->field_key]['#default_value'] = array_shift($terms);
      $form['fields']['fields']['field_root_' . $field->field_key]['#description'] = '';

      foreach ($terms as $children) {
        ++$ajaxId;
        $form['fields']['fields']['_field_root_' . $field->field_key . $ajaxId] = array(
          '#type' => 'select',
          '#groups' => $field->groups,
          '#attributes' => array(
            'class' => 'field_option_children',
            'alt' => $field->field_key
          ),
          '#suffix' => '',
          '#default_value' => $children['value'],
          '#options' => $children['option'],
        );
      }
      $form['fields']['fields']['_field_root_' . $field->field_key.$ajaxId]['#suffix'] = '</div>';
      $form['fields']['fields']['_field_root_' . $field->field_key.$ajaxId]['#description'] = $field->data['input_description'];
    }

    $form['fields']['fields'][$field->field_key] = array(
      '#type' => 'hidden',
      '#default_value' => $default_value,
      '#groups' => $field->groups,
      '#required' => $field->required,
      '#attributes' => array(
        'class' => 'field_select_value field_select_value_' . $field->field_key,
        'alt' => $field->field_key
      )
    );
  } else {
    $form['fields']['fields'][$field->field_key] = array(
      '#title' => t(NULL, $field->name),
      '#type' => 'select',
      '#groups' => $field->groups,
      '#default_value' => !empty($node->fields[$field->field_key]) ? $node->fields[$field->field_key]->tid : '',
      '#required' => $field->required,
      '#options' => field_get_term_options($options[$field->field_id], $field->required),
      '#description' => t(NULL, $field->data['input_description'])
    );
  }
}

/**
 * selects
 */
function field_selects_field_node_form($field, $node, &$form, $ac) {
  global $conf;

  $options = field_get_cache_term($field->module, $field->type);

  if (!empty($conf['field_term_children']) && !empty($conf['field_term_children'][$field->field_id])) {
    dd_add_js(array('fields' => array(
      $field->field_key => array(
        'url' => url('ajax', array('query' => 'fid=' . $field->field_id . '&op=select&module=field'))
      )
    )), 'setting');

    $form['fields']['fields']['field_root_' . $field->field_key] = array(
      '#title' => t(NULL, $field->name),
      '#type' => 'select',
      '#groups' => $field->groups,
      '#required' => $node->fields[$field->field_key] ? false : $field->required,
      '#attributes' => array(
        'class' => 'field_option_children field_option_children_' . $field->field_key,
        'alt' => $field->field_key
      ),
      '#prefix' => '<div class="form_item form_item_select form_item_selects" id="user_edit_form_field_field_root_' . $field->field_key . '">',
      '#field_suffix' => '<input type="button" value="'.t('field', '增加').'" class="form_select_multiple_button" alt="' . $field->field_key . '" />',
      '#options' => field_get_term_options($options[$field->field_id]),
      '#description' => t(NULL, $field->data['input_description'])
    );
    

    if (!empty($_POST) && isset($_POST['fields'][$field->field_key])) {
      $defaults = $_POST['fields'][$field->field_key];
    } else if ($node->fields[$field->field_key]) {
      foreach ($node->fields[$field->field_key] as $o) {
        $defaults[$o->tid] = $o->tid;
      }
    } else {
      $defaults = array();
    }
    
    $description = '';
    
    if ($defaults) {
      foreach ($defaults as $default_value) {
        $form['fields']['fields'][$field->field_key][$default_value] = array(
          '#type' => 'hidden',
          '#default_value' => $default_value,
          '#attributes' => array('class' => 'field_form_selects_option_' . $default_value)
        );
        if ($node->fields[$field->field_key]) {
          $description .= '<span>' . $node->fields[$field->field_key][$default_value]->name;
          $description .= '<a href="javascript:void(0);" alt="' . $default_value;
          $description .= '" onClick="fields.deleteOption($(this));">(' . t('field', '移除') . ')</a></span>';
        }
      }
    }
    
    $form['fields']['fields']['field_root_' . $field->field_key]['#description'] = $field->data['input_description'] . '<div class="field_form_selects_view">' . $description . '</div>';
    $form['fields']['fields']['_multiple_' . $field->field_key] = array(
      '#type' => 'hidden',
      '#default_value' => $node->fields[$field->field_key] ? 0 : "",
      '#required' => $field->required,
      '#attributes' => array(
        'class' => 'field_select_value field_select_value_' . $field->field_key,
        'alt' => $field->field_key
      ),
      '#suffix' => '</div>'
    );
  } else {
    $defaults = NULL;
    if ($node->fields[$field->field_key]) {
      foreach ($node->fields[$field->field_key] as $o) {
        $defaults[$o->tid] = $o->tid;
      }
    }
    $form['fields']['fields'][$field->field_key] = array(
      '#title' => t(NULL, $field->name),
      '#type' => 'selects',
      '#groups' => $field->groups,
      '#default_value' => $defaults,
      '#required' => $field->required,
      '#options' => field_get_term_options($options[$field->field_id], 1),
      '#description' => t(NULL, $field->data['input_description'])
    );
  }
}

/**
 * tags
 */
function field_tags_field_node_form($field, $node, &$form, $ac) {
  $field_value = '';
  if ($node->fields[$field->field_key]) {
    foreach ($node->fields[$field->field_key] as $o) {
      $field_value .= str_replace(array(',', '，'), array('[-]', '[--]'), $o->name) . ',';
    }
  }

  $form['fields']['fields'][$field->field_key] = array(
    '#title' => t(NULL, $field->name),
    '#type' => 'textfield',
    '#groups' => $field->groups,
    '#default_value' => $field_value ? rtrim($field_value, ',') : '',
    '#required' => $field->required,
    '#description' => t(NULL, $field->data['input_description'])
  );
  
  dd_jqui();
  
  if (!empty($field->data['validate']['autocomplete'])) {
    // 提示已有值列表
    $form['fields']['fields'][$field->field_key]['#autocomplete'] = array(
      'url' => url('ajax', array('query' => 'fid=' . $field->field_id . '&op=autotag&module=field')),
      'dom' => '.form_field_tags_autocomplete_' . $field->field_key,
    );
    $form['fields']['fields'][$field->field_key]['#attributes'] = array('class' => 'form_field_tags_autocomplete_' . $field->field_key);
  } else if (!empty($field->data['validate']['type']) && in_array($field->data['validate']['type'], array('custom_ok', 'custom_not'))) {
    // 提示允许值列表
    $dom = 'ui_field_tags_autocomplete_' . $field->field_id;
    $form['fields']['fields'][$field->field_key]['#attributes'] = array('class' => 'ui_field_autocomplete ' . $dom);
    $form['fields']['fields'][$field->field_key]['#autocomplete'] = array(
      'url' => url('ajax', array('query' => 'fid=' . $field->field_id . '&op=auto&module=field')),
      'dom' => '.' . $dom,
      'range' => true
    );
  }
}

/**
 * textfield
 */
function field_textfield_field_node_form($field, $node, &$form, $ac) {
  $form['fields']['fields'][$field->field_key] = array(
    '#title' => t(NULL, $field->name),
    '#type' => $field->field_type,
    '#groups' => $field->groups,
    '#default_value' => empty($node->fields[$field->field_key]) ? '' : $node->fields[$field->field_key]->text,
    '#required' => $field->required,
    '#description' => t(NULL, $field->data['input_description'])
  );

  if (empty($field->data['validate']['type'])) return;

  if (!empty($field->data['validate']['settings']) && !empty($field->data['validate']['settings'][$field->data['validate']['type']])) {
    $v = $field->data['validate']['settings'][$field->data['validate']['type']];
  } else {
    $v = array();
  }
  
  switch ($field->data['validate']['type']) {
    case 'date': case 'date_time':
      $dom = 'ui_date_click_' . $field->field_id;
      if (!empty($v['datepicker'])) {
        $js = array(
          'dom' => '.' . $dom,
          'dateFormat' => $v['value_type'],
        );
        
        if ($field->data['validate']['type'] == 'date_time') {
          $js['showTime'] = true;
        }
        if (!empty($v['changeYear'])) {
          $js['changeYear'] = true;
        }
        if (!empty($v['changeMonth'])) {
          $js['changeMonth'] = true;
        }
       if (!empty($v['yearRange'])) {
          $js['yearRange'] = $v['yearRange'];
        }
        if (!empty($v['maxDate'])) {
          $js['maxDate'] = $v['maxDate'];
        }
        if (!empty($v['minDate'])) {
          $js['minDate'] = $v['minDate'];
        }
        if (!empty($v['start_or_end']) && !empty($v['field_key'])) {
          $js[$v['start_or_end']] = 'ok';
          $map = array('start' => 'end', 'end' => 'start');
          $js[$map[$v['start_or_end']]] = 'input[name=\'fields[' . $v['field_key'] . ']\']';
        }
        dd_add_js(array('uidate' => array($js)), 'setting');
      }
      
      dd_jqui();
      $form['fields']['fields'][$field->field_key]['#attributes'] = array('class' => 'ui_date_click ' . $dom);
    break;
    case 'custom_ok': case 'custom_not':
      $dom = 'ui_field_textfield_autocomplete_' . $field->field_id;
      $form['fields']['fields'][$field->field_key]['#attributes'] = array('class' => 'ui_field_autocomplete ' . $dom);
      $form['fields']['fields'][$field->field_key]['#autocomplete'] = array(
        'url' => url('ajax', array('query' => 'fid=' . $field->field_id . '&op=auto&module=field')),
        'dom' => '.' . $dom,
        'range' => true
      );
  }
}

/**
 * textarea
 */
function field_textarea_field_node_form($field, $node, &$form, $ac) {
  $form['fields']['fields'][$field->field_key] = array(
    '#title' => t(NULL, $field->name),
    '#type' => $field->field_type,
    '#groups' => $field->groups,
    '#default_value' => empty($node->fields[$field->field_key]) ? '' : $node->fields[$field->field_key]->text,
    '#required' => $field->required,
    '#description' => t(NULL, $field->data['input_description'])
  );
}

/**
 * file
 */
function field_file_field_node_form($field, $node, &$form, $ac) {
  // 借助 swfupload 批量上传
  if (!empty($field->data['swfupload']) && module_exists('editor')) {
    $validate = _field_get_file_validate($field, $node);

    $form_field_type = '_' . $field->module . $field->type . $field->field_key;

    if (empty($node->nid)) {
      if (empty($_POST['fields'][$form_field_type])) {
        $validate['ext_type'] = md5($field->module . $field->type . $field->field_key . $GLOBALS['user']->uid . uniqid(mt_rand(), true));
      } else {
        $validate['ext_type'] = $_POST['fields'][$form_field_type];
      }

      $form['fields']['fields'][$form_field_type] = array(
        '#default_value' => $validate['ext_type'],
        '#type' => 'hidden',
        '#constant' => 1,
        '#required' => 1,
      );
    }

    $validate['status'] = 0;

    $form['fields']['fields'][$field->field_key] = array(
      '#title' => t(NULL, $field->name),
      '#groups' => $field->groups,
      '#description' => t(NULL, $field->data['input_description']),
      '#value' => editor_get_swfupload(array(
        'swfupload' => array(
          'file_types' => !empty($validate['filetype']) ? '*.' . implode(';*.', $validate['filetype']) : $validate['filetype'] = '*.*',
          'file_upload_limit' => empty($node->fields[$field->field_key]) ? $field->data['multi'] : $field->data['multi']-count($node->fields[$field->field_key])
        ),
        'validate' => $validate
      ))
    );
    
  } else {
    if ($field->data['multi'] > 1) {
      $multi = !empty($field->data['multi']) ? $field->data['multi'] : 0;
      if (!empty($node->fields[$field->field_key])) $multi = $field->data['multi'] - count($node->fields[$field->field_key]);
      $disabled = $multi < 1 ? true : false;
    } else {
      $multi = NULL;
      $disabled = false;
    }
    $form['fields']['fields'][$field->field_key] = array(
      '#title' => t(NULL, $field->name),
      '#type' => 'file',
      '#groups' => $field->groups,
      '#multi' => $multi,
      '#disabled' => $disabled,
      '#description' => t(NULL, $field->data['input_description']),
      '#required' => !empty($node->fields[$field->field_key]) ? false : $field->required,
    );
  }

  if (!empty($node->fields) && !empty($node->fields[$field->field_key])) {

    if ($field->data['multi'] > 1) {
      $prefix = '<div class="field_file_view field_file_view_multi">';
    } else {
      $prefix = '<div class="field_file_view field_file_view_one">';
    }

    $table = array();

    foreach ($node->fields[$field->field_key] as $file) {
      $rows = array();
      $item = array();
      if (strpos($file->filemime, 'image') !== false) {
        if (!empty($field->data['thumbs']['thumb'])) {
          $item[] = l(
            img(image_get_thumb($file->fid, $file->filepath, $field->data['thumbs']['thumb'])), f($file->filepath),
            array('html' => true, 'attributes' => array('target' => '_blank'))
          );
        } else {
          $item[] = l(img($file->filepath), f($file->filepath), array('html' => true, 'attributes' => array('target' => '_blank')));
        }
      } else {
        $item[] = dd_get_strong(t('field', '文件路径')) . l($file->filepath, f($file->filepath), array(
          'html' => true, 'attributes' => array('target' => '_blank', 'title' => $file->filename)
        ));
      }

      $item[] = dd_get_strong(t('field', '文件大小')) . round($file->filesize/1024/1024, 2) . 'M';
      $item[] = dd_get_strong(t('field', '文件类型')) . $file->filemime;
      $item[] = dd_get_strong(t('field', '上传时间')) . format_date($file->timestamp);
      $rows[] = array('data' => theme('item_list', $item), 'class' => 'field_file_edit_view_table_info');
      
      $content = '<div class="field_file_edit_view_filename">' . dd_form_textfield(array(
        '#name' => 'fields[__field_file_edit_' . $file->fid . '_name]',
        '#value' => $file->filename,
        '#title' => t('field', '文件标题')
      )) . '</div>';

      $content .= '<div class="field_file_edit_view_filebody">' . dd_form_textarea(array(
        '#name' => 'fields[__field_file_edit_' . $file->fid . '_body]',
        '#value' => $file->filebody,
        '#title' => t('field', '详细描述')
      )) . '</div>';

      $rows[] = array('data' => $content, 'class' => 'field_file_edit_view_table_content');

      if ($field->data['multi'] > 1) {
        $rows[] = array(
          'data' => l(t('field', '删除'), 'files/' . $file->fid . '/delete', array(
            'attributes' => array('class' => 'confirmajax', 'level' => 'tr', 'target' => '_blank')
          )),
          'class' => 'field_file_edit_view_table_link'
        );
      }
      $table[] = $rows;
    }

    $header = array(t('field', '文件信息'), t('field', '标题与描述'));
    
    if (count($table[0]) > 2) {
      $header[] = t('field', '操作');
    }

    $form['fields']['fields']['____' . $field->field_key] = array(
      '#type' => 'value',
      '#fieldset_prefix' => 'asc',
      '#fieldset_legend' => t('field', '管理已上传文件'),
      '#fieldset_suffix' => 1,
      '#fieldset' => true,
      '#groups' => $field->groups,
      '#value' => $prefix . theme('table', $header, $table) . '</div>',
    );
  }
}
