<?php
// $Id$

/**
 * @file 字段相关 hook()
 * 
 * hook_field_call_form()
 * hook_field_call_load()
 * hook_field_call_view()
 * hook_field_call_validate()
 * hook_field_call_save()
 * hook_field_call_custom()
 * hook_field_call_links()
 * hook_field_call_export()
 *
 * 触发以上 hook 前，将执行 module_include_file('field/admin.inc')
 * 即，如果模块想实现以上 hook，可将代码放在 .module 文件中，也可以放在模块目录下的 field/admin.inc 文件中
 * 其它模块若要触发以上 hook，必须先执行 module_include_file('field/admin.inc')
 *
 */

/**
 * @Implement of hook_field_call_form()
 *  在添加/更新表单中增加自定义项目
 */
function field_field_call_form($field, &$form) {

  switch ($field->field_type) {
    case 'textarea':
      $form['fields']['data']['format'] = filter_form_field($field->data['format']);

      $form['fields']['data']['validate'] = array(
        '#fieldset_prefix' => 'asc',
        '#fieldset_legend' => t('field', '高级设置'),
        '#fieldset_suffix' => 1,
        '#fieldset' => true,
      );

      $form['fields']['data']['validate']['maxsize'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '最大长度'),
        '#weight' => -3,
        '#default_value' => $field->data['validate']['maxsize'],
        '#description' => t('field', '汉字、字母、数字均算做一个字符')
      );

      $form['fields']['data']['validate']['minsize'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '最小长度'),
        '#weight' => -2,
        '#default_value' => $field->data['validate']['minsize']
      );
    break;

    case 'textfield':
      $form['fields']['data']['validate'] = array(
        '#fieldset_prefix' => 'asc',
        '#fieldset_legend' => t('field', '高级设置'),
        '#fieldset_suffix' => 1,
        '#fieldset' => true,
      );

      $form['fields']['data']['validate']['maxsize'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '最大长度'),
        '#weight' => -4,
        '#default_value' => $field->data['validate']['maxsize'],
        '#description' => t('field', '单行文本最大不超过 255 个字符。汉字、字母、数字均算做一个字符')
      );

      $form['fields']['data']['validate']['minsize'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '最小长度'),
        '#weight' => -3,
        '#default_value' => $field->data['validate']['minsize']
      );

      $form['fields']['data']['validate']['unique'] = array(
        '#type' => 'radio',
        '#title' => t('field', '唯一值'),
        '#weight' => -2,
        '#options' => array(t('field', '否'), t('field', '是')),
        '#default_value' => $field->data['validate']['unique'],
        '#description' => t('field', '不允许重复值')
      );   

      $options = array(t('field', '任意'));
      
      dd_add_js('modules/field/field.admin.js');

      if (!empty($field->data['validate']['settings'])) {
        $value = $field->data['validate']['settings'][$field->data['validate']['type']];
      } else {
        $value = array();
      }
      
      // 自定义允许或禁止值
      $custom_fields = array(
        'value_type' => array(
          '#type' => 'radio',
          '#title' => t('field', '匹配模式'),
          '#default_value' => $value['value_type'],
          '#options' => array(
            1 => t('field', '普通模式'),
            2 => t('field', '正则模式'),
          )
        ),
        'values' => array(
          '#type' => 'textarea',
          '#default_value' => $value['values'],
          '#attributes' => array('rows' => 6, 'cols' => 60),
        )
      );

      $item = array(
        t('field', '模式说明：'),
        t('field', '普通模式：一行一条记录，首尾空格将被清除'),
        t('field', '正则模式：符合PHP标准的正则表达式。以斜杠开头，如：/a.*z/ 表示字符串以a开头，以z结尾')
      );

      // 允许使用 php 输入法的用户
      if ($GLOBALS['_filter_is_php_code'] == true && user_access('filter use php')) {
        $custom_fields['value_type']['#options'][3] = t('field', 'PHP 模式');
        $item[] = t('field', 'PHP模式：不包含 php 标签，必须用 return 而不是 echo，返回 false 或 true 或 数组，false 表示未通过验证。可用变量：$field_value - 当前输入值，$field：当前字段，$field_type：操作类型，$is_ajax：是否为 ajax 验证');
      }

      $custom_fields['values']['#description'] = theme('item_list', $item);

      // 字段值过滤条件
      $data_fields = array(
        'value_type' => array(
          '#type' => 'select',
          '#title' => t('field', '日期格式'),
          '#default_value' => $value['value_type'],
          '#options' => array(
            'yy-mm-dd' => 'yy-mm-dd',
            'mm/dd/yy' => 'mm/dd/yy'
          )
        ),
        'datepicker' => array(
          '#type' => 'radio',
          '#title' => t('field', '日期控件'),
          '#default_value' => isset($value['datepicker']) ? $value['datepicker'] : 1,
          '#options' => array(t('field', '不启用'), t('field', '启用')),
          '#description' => t('field', 'js 日期选择控件，便于用户选择日期。只有启用以下选项方有效')
        ),
        'changeYear' => array(
          '#type' => 'radio',
          '#title' => t('field', '年份下拉'),
          '#default_value' => $value['changeYear'],
          '#options' => array(t('field', '否'), t('field', '是')),
        ),
        'changeMonth' => array(
          '#type' => 'radio',
          '#title' => t('field', '月份下拉'),
          '#default_value' => $value['changeMonth'],
          '#options' => array(t('field', '否'), t('field', '是')),
        ),
        'yearRange' => array(
          '#type' => 'textfield',
          '#title' => t('field', '年份范围'),
          '#default_value' => $value['yearRange'],
          '#description' => t('field', '显示在下拉框中，可以是绝对值，如：1949:2049，或相对值，如：-10:+10'),
        ),
        'maxDate' => array(
          '#type' => 'textfield',
          '#title' => t('field', '最大日期'),
          '#default_value' => $value['maxDate'],
          '#description' => t('field', '可以是绝对值，如：2010-01-22，或相对值，如：+1y +1m +1w +1d'),
        ),
        'minDate' => array(
          '#type' => 'textfield',
          '#title' => t('field', '最小日期'),
          '#default_value' => $value['minDate'],
          '#description' => t('field', '可以是绝对值，如：2008-01-22，或相对值，如：-1y -1m -1w -1d'),
        ),
        'start_or_end' => array(
          '#type' => 'select',
          '#title' => t('field', '字段关联'),
          '#default_value' => $value['start_or_end'],
          '#options' => array(
            t('field', '不需要'),
            'start' => t('field', '做为开始日期'),
            'end' => t('field', '做为结束日期'),
          ),
          '#description' => t('field', '将两个日期字段关联起来，一个做为开始日期，一个做为结束日期'),
        ),
        'field_key' => array(
          '#type' => 'textfield',
          '#title' => t('field', '另一个字段'),
          '#default_value' => $value['field_key'],
          '#description' => t('field', '请输入需要关联的另一个字段的索引名称'),
        ),
      );

      $value_fields = array(
        'date' => array(
          'title' => t('field', '日期'),
          'fields' => $data_fields
        ),
        'date_time' => array(
          'title' => t('field', '日期和时间'),
          'fields' => $data_fields
        ),
        'mail' => array(
          'title' => t('field', '电子邮件')
        ),
        'figure' => array(
          'title' => t('field', '只允许数字'),
        ),
        'letter' => array(
          'title' => t('field', '只允许字母'),
        ),
        'figure_letter' => array(
          'title' => t('field', '数字字母混合'),
        ),
        'chinese' => array(
          'title' => t('field', '只允许中文'),
        ),
        'url' => array(
          'title' => t('field', '只允许网址'),
        ),
        'custom_ok' => array(
          'title' => t('field', '自定义(允许特定值)'),
          'fields' => $custom_fields,
        ),
        'custom_not' => array(
          'title' => t('field', '自定义(不允许特定值)'),
          'fields' => $custom_fields,
        ),
      );
       
      if (!empty($_POST['data']) && !empty($_POST['data']['validate'])) {
        $data_type = $_POST['data']['validate']['type'];
      } else {
        $data_type = $field->data['validate']['type'];
      }

      foreach ($value_fields as $key => $o) {
        $options[$key] = $o['title'];

        if (empty($o['fields'])) continue;

        foreach ($o['fields'] as $field_key => $c) {
          unset($o['fields']['#required']);
        }
        
        // 自定义配置字段
        $form['fields']['data']['validate']['settings'][$key] = $o['fields'];
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_prefix'] = 'asc';
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_legend'] = t('field', '自定义参数');
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_suffix'] = 1;
        $form['fields']['data']['validate']['settings'][$key]['#fieldset'] = true;
        
        if ($data_type == $key) {
          $display = 'block';
        } else {
          $display = 'none';
        }
        
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_attributes'] = array(
          'class' => 'field_type_textfield_settings field_type_textfield_settings_' . $key,
          'style' => 'display: ' . $display . ';'
        );
      }

      $form['fields']['data']['validate']['type'] = array(
        '#type' => 'select',
        '#title' => t('field', '输入类型'),
        '#default_value' => $field->data['validate']['type'],
        '#options' => $options,
        '#weight' => -1,
        '#description' => t('field', '此字段允许输入的值。')
      );
    break;

    case 'file':
      $form['fields']['data']['multi'] = array(
        '#type' => 'select',
        '#title' => t('field', '数量'),
        '#default_value' => $field->data['multi'] ? $field->data['multi'] : 0,
        '#options' => array(1 => 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 50 => 50),
        '#description' => t('field', '可同时上传多个文件，默认为  1 个')
      );

      $form['fields']['data']['swfupload'] = array(
        '#type' => 'checkbox',
        '#title' => '工具',
        '#default_value' => $field->data['swfupload'] ? $field->data['swfupload'] : 0,
        '#description' => t('field', '使用 swfupload 可以让批量上传更便捷，需要先启用 editor 模块'),
        '#disabled' => !module_exists('editor')
      );
   
      $form['fields']['data']['filetype'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '类型'),
        '#default_value' => $field->data['filetype'] ? implode('|', $field->data['filetype']) : '',
        '#description' => t('field', '允许的文件类型，留空为不限。以|分隔，如：jpg|gif')
      );

      $form['fields']['data']['filesize'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '大小'),
        '#default_value' => $field->data['filesize'],
        '#description' => t('field', '允许的文件大小，单位KB，留空为不限')
      );

      $form['fields']['data']['filename'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '名称'),
        '#default_value' => $field->data['filename'],
        '#description' => t('field', '指定文件名称，可使用<a href="!url">通配符</a>，建议留空', array('!url' => url('admin/token')))
      );

      $form['fields']['data']['extension'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '扩展'),
        '#default_value' => $field->data['extension'],
        '#description' => t('field', '指定文件扩展名，建议留空')
      );

      $form['fields']['data']['thumbs']['thumb'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '小图'),
        '#default_value' => $field->data['thumbs']['thumb'] ? $field->data['thumbs']['thumb'] : '85x85',
        '#description' => t('field', '缩略图尺寸，如：85x85')
      );

      $form['fields']['data']['thumbs']['medium'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '中图'),
        '#default_value' => $field->data['thumbs']['medium'] ? $field->data['thumbs']['medium'] : '200x200',
        '#description' => t('field', '适中图尺寸，如：200x200')
      );

      $form['fields']['data']['savepath'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '路径'),
        '#default_value' => $field->data['savepath'],
        '#description' => t('field', '文件保存路径，可使用<a href="!url">通配符</a>，空则使用用户默认文件路径', array('!url' => url('admin/token')))
      );
    break;

    case 'tags':
      $form['fields']['data']['validate'] = array(
        '#fieldset_prefix' => 'asc',
        '#fieldset_legend' => t('field', '高级设置'),
        '#fieldset_suffix' => 1,
        '#fieldset' => true,
      );

      $form['fields']['data']['validate']['autocomplete'] = array(
        '#type' => 'checkbox',
        '#title' => t('field', '输入提示'),
        '#weight' => -4,
        '#default_value' => $field->data['validate']['autocomplete'],
        '#description' => t('field', '根据用户输入，从已有的记录中查询并提示')
      );

      $form['fields']['data']['validate']['maxsize'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '最大长度'),
        '#weight' => -3,
        '#default_value' => $field->data['validate']['maxsize'],
        '#description' => t('field', '标签最大不超过 64 个字符。汉字、字母、数字均算做一个字符')
      );

      $form['fields']['data']['validate']['minsize'] = array(
        '#type' => 'textfield',
        '#title' => t('field', '最小长度'),
        '#weight' => -2,
        '#default_value' => $field->data['validate']['minsize']
      );
      
      $options = array(t('field', '任意'));
      
      dd_add_js('modules/field/field.admin.js');

      if (!empty($field->data['validate']['settings'])) {
        $value = $field->data['validate']['settings'][$field->data['validate']['type']];
      } else {
        $value = array();
      }
      
      // 自定义允许或禁止值
      $custom_fields = array(
        'value_type' => array(
          '#type' => 'radio',
          '#title' => t('field', '匹配模式'),
          '#default_value' => $value['value_type'],
          '#options' => array(
            1 => t('field', '普通模式'),
            2 => t('field', '正则模式'),
          )
        ),
        'values' => array(
          '#type' => 'textarea',
          '#default_value' => $value['values'],
          '#attributes' => array('rows' => 6, 'cols' => 60),
        )
      );

      $item = array(
        t('field', '模式说明：'),
        t('field', '普通模式：一行一条记录，首尾空格将被清除'),
        t('field', '正则模式：符合PHP标准的正则表达式。以斜杠开头，如：/a.*z/ 表示字符串以a开头，以z结尾')
      );

      // 允许使用 php 输入法的用户
      if ($GLOBALS['_filter_is_php_code'] == true && user_access('filter use php')) {
        $custom_fields['value_type']['#options'][3] = t('field', 'PHP 模式');
        $item[] = t('field', 'PHP模式：不包含 php 标签，必须用 return 而不是 echo，返回 false 或 true 或 数组，false 表示未通过验证。可用变量：$field_value - 当前输入值，$field：当前字段，$field_type：操作类型，$is_ajax：是否为 ajax 验证');
      }

      $custom_fields['values']['#description'] = theme('item_list', $item);

      $value_fields = array(
        'custom_ok' => array(
          'title' => t('field', '自定义(允许特定值)'),
          'fields' => $custom_fields,
        ),
        'custom_not' => array(
          'title' => t('field', '自定义(不允许特定值)'),
          'fields' => $custom_fields,
        ),
      );     

      if (!empty($_POST['data']) && !empty($_POST['data']['validate'])) {
        $data_type = $_POST['data']['validate']['type'];
      } else {
        $data_type = $field->data['validate']['type'];
      }
      
      foreach ($value_fields as $key => $o) {
        $options[$key] = $o['title'];

        if (empty($o['fields'])) continue;
        
        foreach ($o['fields'] as $field_key => $c) {
          unset($o['fields']['#required']);
        }
        
        // 自定义配置字段
        $form['fields']['data']['validate']['settings'][$key] = $o['fields'];
        $coustom_field = true;
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_prefix'] = 'asc';
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_legend'] = t('field', '自定义参数');
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_suffix'] = 1;
        $form['fields']['data']['validate']['settings'][$key]['#fieldset'] = true;
        
        if ($data_type == $key) {
          $display = 'block';
        } else {
          $display = 'none';
        }
        
        $form['fields']['data']['validate']['settings'][$key]['#fieldset_attributes'] = array(
          'class' => 'field_type_textfield_settings field_type_textfield_settings_' . $key,
          'style' => 'display: ' . $display . ';'
        );
      }

      $form['fields']['data']['validate']['type'] = array(
        '#type' => 'select',
        '#title' => t('field', '输入类型'),
        '#default_value' => $field->data['validate']['type'],
        '#options' => $options,
        '#weight' => -1,
        '#description' => t('field', '此字段允许输入的值。')
      );
    break;
  }
  
  if (!empty($field->field_id) && in_array($field->field_type, field_get_term_type())) {
    /**
     * 编辑字段，是否允许在不同类型间变更？如把单行文本改为多行文本
     * 不同字段数据结构、存储方式不同，变更会引发不同的问题
     * 但传统型字段（即分类）常用类型变更需求，特别是新开设站点
     * 所以允许在这几个类型间变更，但可能造成未知问题。
     * 如自由标签变更为单选，若分类下已存在数据，将难以自动转换
     * 通常在字段下已有数据时，不应该变更它
     */
    $options = field_get_type_options($field->type, $field->type); // 允许的类型
    $form['fields']['field_type'] = array(
      '#type' => 'select',
      '#title' => t('field', '类型'),
      '#required' => 1,
      '#default_value' => $field->field_type,
      '#options' => array_intersect_key(field_get_type_options('___term', array('term' => 1)), $options),
    );
    unset($form['fields']['_field_type']);
  }
}

/**
 * @Implement of hook_field_call_validate()
 *  添加\更新字段通过基本验证后回调
 * @return
 *  返回 array 或 false。array 包含一组错误信息，阻止保存操作。false 则表示通过验证
 */
function field_field_call_validate(&$field) {
  switch ($field->field_type) {
    case 'file':
      if (!empty($field->data) && !empty($field->data['filetype'])) {
        if (strpos($field->data['filetype'], '|') !== false) {
          $field->data['filetype'] = explode('|', $field->data['filetype']);
        } else {
          $field->data['filetype'] = array($field->data['filetype']);
        }
      }
      if ($field->data['filesize'] && !is_numeric($field->data['filesize'])) {
        dd_set_message(t('field', '文件大小只能是数字'), 'error');
        return true;
      }
    break;
    case 'validate':
      // 字段验证
      if ($field->data['validate']['settings']['start_or_end'] && ($type == 'date' || $type == 'date_time')) {
        if ($field->data['validate']['settings']['field_key'] == $field->field_key) {
          dd_set_message(t('field', '不能关联自己'), 'error');
          return true;
        }
      }
    break;
  }
}

/**
 * @Implement of hook_field_call_links()
 * 自定义字段管理界面链接，
 */
function field_field_call_links($field, &$links = NULL, $get = NULL) {
  if (in_array($field->field_type, field_get_term_type())) {
    $links['view_children'] = l(t('field', '浏览子级'), $_GET['q'], array('query' => $get.'field_op=custom&field_custom=view&field_field_id='.$field->field_id));
    $links['add_children'] = l(t('field', '添加子级'), $_GET['q'], array('query' => $get.'field_op=custom&field_custom=add&redirect='.rawurlencode($_SERVER['REQUEST_URI']).'&field_field_id='.$field->field_id));
    $links['add_batch'] = l(t('field', '批量添加'), $_GET['q'], array('query' => $get.'field_op=custom&field_custom=import&redirect='.rawurlencode($_SERVER['REQUEST_URI']).'&field_field_id='.$field->field_id));
  }
}

/**
 * @Implement of hook_field_call_custom()
 * field 只提供基本数据的操作接口及管理界面，即对 fields 表的操作
 * 有些类型可能需要提供界面和操作，故而在字段管理界面预留路径，用做自定义
 * 在字段管理界面下，当 $_GET['field_op'] == 'custom'，将回调函数
 * 以下主要为增强传统分类型字段的功能，便于通过界面管理
 * field_term_save() 等函数也可供模块直接操作上述类型的字段
 */
function field_field_call_custom($field) {
  if (in_array($field->field_type, field_get_term_type())) {
    if (!empty($_GET['field_term_tid'])) {
      $term = field_term_load($_GET['field_term_tid']);
    }
    switch ($_GET['field_custom']) {
      case 'view':
      return field_term_lists($field, $term);
      case 'import':
      return dd_get_form('field_term_import', $field, $term);
      case 'add':
      return dd_get_form('field_term_form', $field, $term);
      case 'update':
      return dd_get_form('field_term_form', $field, $term);
      case 'delete':
        if ($term && field_term_delete($term)) {
          dd_set_message(t('field', '%string 及其子级已成功删除', array('%string' => $term->name)));
        } else {
          dd_set_message(t('field', '删除失败'), 'error');
        }
        dd_goto(dd_get_history());
    }
  }
}

/**
 * @Implement of hook_field_call_save()
 *  字段添加\更新成功后回调
 */
function field_field_call_save($field) {
  /** 
   * 字段类型的创建者（模块）可为字段定义多个参数，在字段基本信息存储成功后，自行写入
   * 如本模块的传统型字段即定义了 $field->children，它包含一个分类树。格式如：
      $field->children = array(
        array(
          'name' => '中国',
          'weight' => 10,
          'childrens' => array(
            array(
              'name' => '四川',
              'childrens' => array(
                array(
                  'name' => '成都'
                ),
              )
            ),
          )
        )
      );
   * 或 $field->children = array('中国', '美国', '法国');
   * 这个分类树，最多不超过 10 级，在定义字段组时，可以设置层级限制
   * 字段基本信息添加成功后，若有此值，则写入 field_term 表中
   */
  if ($field->children && in_array($field->field_type, field_get_term_type())) {
   field_term_batch_save($field, 0, $field->children);
  }
  if ($field->data['validate']['type']) {
      // 回调自定义配置
    module_invoke_all('field_type_'.$field->field_type, 'save', $field->data['validate']['type'], $field);
  }
}

/**
 * @Implement of hook_field_call_export()
 */
function field_field_call_export(&$field) {
  
  if (in_array($field->field_type, field_get_term_type())) {
    if ($fetch = db_query('SELECT tid, pid, weight, name, description FROM {fields_term} WHERE field_id = ?',
    array($field->field_id), array('fetch' => 'array'))) {
      
      foreach ($fetch as $o) {
        $terms[$o['pid']][$o['tid']] = $o;
      }
      
      foreach ($terms[0] as $a) {
        $rows = $a;
        if ($sub = _field_field_call_export($a['tid'], $terms)) {
          $rows['childrens'] = $sub;
        }
        $items[] = $rows;
      }
      
      if ($items) {
        $field->children = $items;
      }
    }
  }
}

function _field_field_call_export($tid, $terms) {
  if ($terms[$tid]) {
    foreach ($terms[$tid] as $a) {
      $rows = $a;
      if ($sub = _field_field_call_export($a['tid'], $terms)) {
        $rows['childrens'] = $sub;
      }
      $items[] = $rows;
    }
  }
  return $items;
}

/////////////////

/**
 * 分类管理界面
 */
function field_term_lists($field, $term = NULL) {
  $group = module_invoke($field->module, 'field_call_group', $field->type);

  $sub_tabs = array(l(t('field', '返回管理'), $_GET['q']));

  $get = dd_query_string_encode($_GET, array_merge(array('q', 'field_term_tid', 'field_custom', 'field_term_lists'), array_keys($_COOKIE)));
  
  $redirect = '&redirect='.rawurlencode($_SERVER['REQUEST_URI']);
  
  if ($term->tid) {
    $sub_tabs[] = l(t('field', '返回上级'), $_GET['q'], array('query' => $get.'&field_custom=view&field_term_tid='.$term->pid));
    $sub_tabs[] = l(t('field', '添加'), $_GET['q'], array('query' => $get.'&field_custom=add&field_term_pid='.$term->tid.$redirect));
  } else {
    $term->tid = 0;
    $sub_tabs[] = l(t('field', '添加'), $_GET['q'], array('query' => $get.'&field_custom=add&'.$redirect));
    $sub_tabs['level'] = l(t('field', '树形显示全部分类'), $_GET['q'], array('query' => $get.'&field_custom=view&field_term_lists=1'));
  }
  
  dd_set_title(array($term->name, $field->name, $group['title']));
  
  if (!$_GET['field_term_lists']) {
    $fetch = pager_query('SELECT * FROM {fields_term} WHERE field_id = ? AND pid = ?', 20, 0, NULL, array($field->field_id, $term->tid));
  } else if ($options = field_term_options($field, field_group_info_level($group, $field), 1)) {
    array_shift($options);
    $fetch = (object) $options;
    $sub_tabs['level'] = l(t('field', '分页显示分类'), $_GET['q'], array('query' => $get.'&field_custom=view'));
  }
  if ($fetch) {
    $header = array(
      t('field', '名称'), t('field', '操作')
    );
    
    foreach ($fetch as $o) {
      $links = array(
        l(t('field', '编辑'), $_GET['q'], array('query' => $get.'&field_custom=update&field_term_tid='.$o->tid.$redirect)),
        l(t('field', '删除'), $_GET['q'], array('query' => $get.'&field_custom=delete&field_term_tid='.$o->tid, 'attributes' => array('class' => 'confirm'))),
      );

      if (!$_GET['field_term_lists']) {
        if (field_term_is_level($o->tid, field_group_info_level($group, $field))) {
          $links[] = l(t('field', '浏览子级'), $_GET['q'], array('query' => $get.'&field_custom=view&field_term_tid='.$o->tid));
          $links[] = l(t('field', '添加子级'), $_GET['q'], array('query' => $get.'&field_custom=add&field_term_pid='.$o->tid.$redirect));
          $links[] = l(t('field', '批量添加'), $_GET['q'], array('query' => $get.'&field_custom=import&field_term_pid='.$o->tid.$redirect));
        }
      } else {
        $links[] = l(t('field', '添加子级'), $_GET['q'], array('query' => $get.'&field_custom=add&field_term_pid='.$o->tid.$redirect));
        $links[] = l(t('field', '批量添加'), $_GET['q'], array('query' => $get.'&field_custom=import&field_term_pid='.$o->tid.$redirect));     
      }
      
      $table[] = array($o->name, dd_imp($links));
    }
    $output = theme('table', $header, $table);
    $output .= pager_view();
  } else {
    $output = system_no_content();
  }
  
  dd_set_sub_tabs($sub_tabs);
  return $output;
}


