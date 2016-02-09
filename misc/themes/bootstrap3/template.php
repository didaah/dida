<?php
// $Id$

/**
 * @Implement of hook_template_framework_x()
 */
function bootstrap3_template_framework_help($help) {
  return '<div class="alert alert-block alert-info help"><button type="button" class="close" data-dismiss="alert">&times;</button>' . implode('', $help) . '</div>';
}

/**
 * @Implement of hook_template_framework_x()
 */
function bootstrap3_template_framework_table(array $header, $rows = array(), array $attributes = array(), $caption = NULL, $sub_header = NULL) {
  if (!empty($attributes['class'])) {
    $attributes['class'] .= ' table table-bordered table-condensed';
  } else {
    $attributes['class'] = 'table table-bordered table-condensed';
  }

  $output = '<table'. dd_attributes($attributes) .">\n";

  if (isset($caption)) {
    $output .= '<caption>'. $caption ."</caption>\n";
  }

  if (!empty($header)) {
    $ts = table_init($header);
    if (!empty($rows)) $output .= '<thead>';
    $output .= '<tr>';
    foreach ($header as $cell) {
      $cell = table_header($cell, $header, $ts);
      $output .= _theme_table_cell($cell, true);
    }
    $output .= '</tr>';

    if (!empty($sub_header)) {
      foreach ($sub_header as $row) {
        $attributes = array();
        if (isset($row['data'])) {
          foreach ($row as $key => $value) {
            if ($key == 'data') {
              $cells = $value;
            } else {
              $attributes[$key] = $value;
            }
          }
        } else {
          $cells = $row;
        }
        $i = 0;
        $output .= '<tr' . dd_attributes($attributes) . '>';
        foreach ($cells as $cell) {
          $cell = table_cell($cell, $header, $ts, $i++);
          $output .= _theme_table_cell($cell, true);
        }
        $output .= '</tr>';
      }
    }

    if (!empty($rows)) $output .= '</thead>';
  } else {
    $ts = array();
  }

  if (!empty($rows)) {
    $output .= "<tbody>\n";
    $flip = array('even' => 'odd', 'odd' => 'even');
    $class = 'even';
    foreach ($rows as $number => $row) {
      $attributes = array();

      if (isset($row['data'])) {
        foreach ($row as $key => $value) {
          if ($key == 'data') {
            $cells = $value;
          } else {
            $attributes[$key] = $value;
          }
        }
      } else {
        $cells = $row;
      }
      if (count($cells)) {
        $class = $flip[$class];
        if (isset($attributes['class'])) {
          $attributes['class'] .= ' '. $class;
        } else {
          $attributes['class'] = $class;
        }
        $output .= ' <tr'. dd_attributes($attributes) .'>';
        $i = 0;
        foreach ($cells as $cell) {
          $cell = table_cell($cell, $header, $ts, $i++);
          $output .= _theme_table_cell($cell);
        }
        $output .= " </tr>\n";
      }
    }
    $output .= "</tbody>\n";
  }

  $output .= "</table>\n";

  return '<div class="table-responsive">' . $output . '</div>';
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_element_tabs($tabs) {
  $output = '<ul id="tabs" class="nav nav-tabs">';
  foreach ($tabs as $value) {
    $output .= '<li>' . $value . '</li>';
  }
  $output .= '</ul>';
  return $output;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_get_menu($menus = NULL, $first = true) {
  if (!isset($menus)) {
    $menus = array();
    module_alter_all('site_menu', $menus);
    $class = 'site_menu nav navbar-nav';
  } else {
    $class = 'site_menu_children dropdown-menu';
  }

  if ($menus) {
    static $i;

    if (!empty($first)) {
      $custom_data = var_get('site_menu_system', array());
    }

    foreach ($menus as $key => $lists) {
      if (!empty($first)) {
        // 已禁用的导航
        if (!empty($custom_data['disabled']) && isset($custom_data['disabled'][$key])) continue;
        
        // 可通过 hook_site_menu_alter() 覆写 menu
        module_alter_all('site_menu_alter', $key, $lists);

        // 自定义权重
        if (!empty($custom_data['weight']) && !empty($custom_data['weight'][$key])) {
          if (is_array($lists)) {
            $lists['#weight'] = $custom_data['weight'][$key];
          } else {
            $lists = array(
              '#data' => $lists,
              '#weight' => $custom_data['weight'][$key],
            );
          }
        }
      }

      if (is_array($lists)) {
        if ($lists['#data']) {
          $data = '';
          $data .= $lists['#data'];
          if ($lists['#childrens']) {
            $data .= framework_element_dd_get_menu($lists['#childrens'], 0);
          }
          $items[] = array(
            '#data' => array('data' => $data, 'class' => 'site_menu_list site_menu_list_' . $key),
            '#weight' => ($lists['#weight'] ? $lists['#weight'] : $weight)
          );
        } else if (!$lists['#childrens']) {
          foreach ($lists as $k => $child) {
            if (is_array($child)) {
              if ($child['#data']) {
                $data = '';
                $data .= $child['#data'];
                if ($child['#childrens']) {
                  $data .= framework_element_dd_get_menu($child['#childrens'], 0);
                }
                $items[] = array(
                  '#data' => array('data' => $data, 'class' => 'site_menu_list site_menu_list_' . $k),
                  '#weight' => ($child['#weight'] ? $child['#weight'] : $weight)
                );
              }
            } else {
              $items[] = array(
                '#data' => array('data' => $child, 'class' => 'site_menu_list site_menu_list_' . $k),
                '#weight' => $i
              );
            }
            ++$i;
          }
        }
      } else {
        $items[] = array(
          '#data' => array('data' => $lists, 'class' => 'site_menu_list site_menu_list_' . $key),
          '#weight' => $i,
        );
      }
      ++$i;
    }

    if ($items) {
      uasort($items, 'dd_form_cmp');
      $j = 1;
      foreach ($items as $d) {
        if (!empty($first)) {
          $d['#data']['class'] .= ' site_menu_list_index_' . $j;
        } else {
          $d['#data']['class'] .= ' site_menu_list_index_sub_' . $j;
        }
        $item[] = $d['#data'];
        $j++;
      }
      $output = theme('item_list', $item, NULL, 'ul', array('class' => $class), 0);
    }
    return $output;
  }
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_element_sub_tabs($tabs) {
  $output = '<ul id="sub_tabs" class="nav nav-pills">';
  foreach ($tabs as $value) {
    $output .= '<li>' . $value . '</li>';
  }
  $output .= '</ul>';
  return $output;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_element_message($messages) {
  $types = array(
    'notice' => 'alert-info',
    'success' => 'alert-success',
    'error' => 'alert-error alert-danger',
    'warning' => 'alert-error',
  );

  $output = '';
  foreach ($messages as $type => $data) {
    $output .= '<div class="messages alert alert-block ' . $types[$type] . '">';
    $output .= '<button type="button" class="close" data-dismiss="alert">&times;</button>';
    if (count($data) > 1) {
      $output .= " <ul>\n";
      foreach ($data as $message) {
        $output .= '  <li>'. $message ."</li>\n";
      }
      $output .= " </ul>\n";
    } else {
      $output .= $data[0];
    }
    $output .= "</div>\n";
  }
  return $output;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_form_button($field) {
  if (!empty($field['#attributes']['class'])) {
    $field['#attributes']['class'] = ' btn btn-default';
  } else {
    $field['#attributes']['class'] = 'btn btn-default';
  }
  $output = '<input type="submit" name="' . $field['#name'] . '" value="';
  $output .= ($field['#value'] ? $field['#value'] : t('system', '确认提交'));
  $output .= '"' . dd_attributes($field['#attributes']) . '/>';
  return $output;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_form_submit($field) {
  if (!empty($field['#attributes']['class'])) {
    $field['#attributes']['class'] = ' btn btn-primary';
  } else {
    $field['#attributes']['class'] = 'btn btn-primary';
  }
  $output = '<input type="submit" name="' . $field['#name'] . '" value="';
  $output .= ($field['#value'] ? $field['#value'] : t('system', '确认提交'));
  $output .= '"' . dd_attributes($field['#attributes']) . '/>';
  return $output;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_form_label($field) {
  $output = '';
  if (!empty($field['#title'])) {
    if (empty($field['#required'])) {
      $output = '<label class="dd_label control-label dd_label_' . $field['#type'] . '" for="' . $field['#name'] . '">';
    } else {
      $output = '<label class="dd_label control-label form_required dd_label_' . $field['#type'] . '" for="' . $field['#name'] . '"';
      $output .= ' title="' . t('system', '此项不能为空') . '">';
    }
    $output .= $field['#title'] . '：';
    $output .= '</label>';
  }
  return $output;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_form_get_html($form) {
  $output = '';
  //ajax验证和提交将跳过系统默认验证，应在ajax页面调用dd_ajax_validate()来验证
  if (!empty($form['settings']['#ajax_submit']) && !empty($form['settings']['#ajax_submit']['des'])) {
    $output = '<div id="ajax_description" class="description"></div>';
  }

  //$form['#args']['class'] = 'form-horizontal';

  $output .= '<form ' . dd_attributes($form['#args']) . '>';

  if (!empty($form['settings']['#title'])) {
    $output .= '<h3>' . $form['settings']['#title'] . '</h3>';
  }

  if (!empty($form['settings']['#description'])) {
    $output .= '<div class="description form_description form_setting_description">' . $form['settings']['#description'] . '</div>';
  }

  $output .= $form['#content'];
  $output .= '</form>';
  return $form['settings']['#prefix'] . $output . $form['settings']['#suffix'];
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_form_wrapper_element($field, $form) {
  if (empty($form['#horizontal'])) {
    $element = $field['#element']['#label'];
  } else {
    $element = '<div class="control-group">' . $field['#element']['#label'] . '<div class="controls">';
  }

  if (!empty($field['#element']['#field_prefix'])) {
    $element .= $field['#element']['#field_prefix'];
  }
  $element .= $field['#element']['#content'];
  if (!empty($field['#element']['#field_suffix'])) {
    $element .= '<span class="add-on">' . $field['#element']['#field_suffix'] . '</span>';
  }
  $element .= $field['#element']['#error'];
 
  if (!empty($field['#element']['#description'])) {
    $element .= '<span class="description form_description help-inline">' . $field['#element']['#description'] . '</span>';
  }
  
  if (!empty($form['#horizontal'])) {
    $element .= '</div></div>';
  }

  if (isset($field['#prefix']) || isset($field['#suffix'])) {
    $element = $field['#element']['#prefix'] . $element . $field['#element']['#suffix'];
  } else if (empty($form['settings']['#theme']) && $field['#type'] != 'hidden') {
    $element = '<div id="' . $field['#form_id'] . '_' . $field['__name'] . '" class="form_item form_item_' . $field['#type'] . '">' . $element . '</div>';
  }

  $field['#html'] = $field['#element']['#fieldset_prefix_html'] . $element . $field['#element']['#fieldset_suffix_html'];
  return $field;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_form_checkbox($field, $form) {
  $output = '';
  
  if (!empty($field['#options'])) {
    $id = $field['#attributes']['id'];
    $field['#attributes']['class'] .= ' ' . $id;

    foreach ($field['#options'] as $key => $data) {
      $output .= '<label class="checkbox-inline checkbox_field_' . $field['#name'] . '">';
      $output .= '<span class="checkbox inline form_checkbox_option" alt="' . $id . '"><input';

      if (is_array($field['#value']) && in_array($key, $field['#value'])) {
        $output .= ' checked="checked"';
      }

      $field['#attributes']['id'] = $id .'_'. $key;
      
      $att = $field['#attributes'];
      $att['class'] .= ' checkbox';

      if (is_array($data)) {
        foreach ($data as $_key => $value) {
          if ($_key == 'data' || $_key == 'name') {
            $text = $value;
          } else {
            $att[$_key] = $value;
          }
        }
      } else {
        $text = $data;
      }

      $output .= ' value="' . $key . '" name="' . $field['#name'] . '[' . $key . ']" type="checkbox"';

      $output .= dd_attributes($att) . '/><span class="option_label" title="' . strip_tags($text) . '">' . $text . '</span></span></label>';
    }
  } else {
    $output .= '<label class="checkbox-inline checkbox_field_' . $field['#name'] . '"><input';
    if ($field['#value']) $output .= ' checked="checked"';
    $output .= ' value="1" name="' . $field['#name'] . '" type="checkbox"' . dd_attributes($field['#attributes']) . '/></label>';
  }

  return $output;
}

/**
 * @Implement of framework_element_x()
 */
function framework_element_dd_form_radio($field, $form) {
  $output = '';

  if (is_array($field['#options'])) {
    $id = $field['#attributes']['id'];

    if (!empty($field['#attributes']['class'])) {
      $field['#attributes']['class'] .= ' ' . $id;
    } else {
      $field['#attributes']['class'] = $id;
    }

    foreach ($field['#options'] as $key => $data) {
      $field['#attributes']['id'] = $id . '_' . $key;
      $att = $field['#attributes'];

      if (is_array($data)) {
        foreach ($data as $_key => $value) {
          if ($_key == 'data' || $_key == 'name') {
            $text = $value;
          } else {
            $att[$_key] = $value;
          }
        }
      } else {
        $text = $data;
      }

      $output .= '<label class="radio-inline radio_field_' . $field['#name'] . '"><span class="form_radio_field"><input';
      
      if ((string) $field['#value'] == (string) $key) $output .= ' checked="checked"';

      $output .= ' value="' . $key . '" name="' . $field['#name'] . '" type="radio"';
      $output .= dd_attributes($att) . '/>';
      $output .= '<span class="form_radio_text" title="' . strip_tags($text) . '">' . $text . '</span></span></label>';
    }
  } else {
    $output .= '<label class="radio-inline radio_field_' . $field['#name'] . '"><span class="form_radio_field"><input';
    if ($field['#value']) $output .= ' checked="checked"';
    $output .= ' value="1" name="'.$field['#name'] . '" type="radio"' . dd_attributes($field['#attributes']);
    $output .= '/></span></label>';
  }

  return $output;
}

