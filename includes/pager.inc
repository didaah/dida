<?php
// $Id$

/**
 * 数据库分页查询
 * @param (string) $query
 *  sql 语句
 * @param (int) $limit
 *  每页数量
 * @param (int) $element
 *  分页标识，当同一页面有多个分页，需指定标识
 * @param (int) $count
 *  数据总记录，若不指定，则分析 $query 运行一个查询
 * @param (array) $args
 *  查询参数，见 db_query()
 * @param (array) $options
 *  查询设置，见 db_query()
 * @return
 *  返回 PDO 结果集
 */
function pager_query($query, $limit = 10, $element = 0, $count = NULL, $args = array(), $options = array()) {
  global $pager_page_array, $pager_total, $pager_total_items;
  
  $page = isset($_GET['page']) ? $_GET['page'] : '';
  
  if (!isset($count)) {
    $count_query = preg_replace(array('/SELECT.*?FROM /As', '/ORDER BY .*/'), array('SELECT COUNT(*) FROM ', ''), $query);
    if ($options) {
      $count_opt = $options;
    }
    
    $count_opt['return'] = 'column';
    
    if (!$pager_total_items[$element] = db_query($count_query, $args, $count_opt)) {
      return false;
    }
    
  } else {
    $pager_total_items[$element] = $count;
  }
  
  $pager_page_array = explode(',', $page);
  $pager_total[$element] = ceil($pager_total_items[$element] / $limit);
  $pager_page_array[$element] = max(0, min((int)$pager_page_array[$element], ((int)$pager_total[$element]) - 1));
  
  $options['limit'] = array($pager_page_array[$element] * $limit, $limit);
  
  return db_query($query, $args, $options);
}

/**
 * 非数据库分页
 * @param (int) $count_query
 *  总记录
 * @param (int) $limit
 *  每页数量
 * @param (int) $element
 *  分页标识
 */
function pager_lists($count, $limit = 10, $element = 0) {
  global $pager_page_array, $pager_total, $pager_total_items;
  $page = isset($_GET['page']) ? $_GET['page'] : '';
  $pager_total_items[$element] = $count;
  $pager_page_array = explode(',', $page);
  $pager_total[$element] = ceil($pager_total_items[$element] / $limit);
  $pager_page_array[$element] = max(0, min((int)$pager_page_array[$element], ((int)$pager_total[$element]) - 1));
}

/**
 * 转换分页链接，保留除 q、page 外的所有 $_REQUEST 参数
 */
function pager_get_querystring() {
  static $string = NULL;
  if (!isset($string)) {
    $string = dd_query_string_encode($_REQUEST, array_merge(array('q', 'page'), array_keys($_COOKIE)));
  }
  return $string;
}

/**
 * 分页 HTML 格式化
 * @param (array) $tags
 *  自定义分页显示文字及其它显示配置，见函数内容 $default_tags
 * @param (int) $limit
 *  每页数量，对应 pager_query 或 pager_lists
 * @param (int) $element
 *  分页标识，对应 pager_query 或 pager_lists
 * @param (array) $parameters
 *  过滤 $_REQUEST 参数，一般默认即可
 * @param (int) quantity
 *  显示页码链接的数量
 * @param (array) $att
 *  链接属性，如：alt、title 等
 */
function pager_view(array $tags = array(), $limit = 10, $element = 0, $parameters = array(), $quantity = 7, $att = array()) {
  global $pager_page_array, $pager_total, $pager_total_items;
  
  $pager_middle = ceil($quantity / 2);
  $pager_current = $pager_page_array[$element] + 1;
  $pager_first = $pager_current - $pager_middle + 1;
  $pager_last = $pager_current + $quantity - $pager_middle;
  $pager_max = $pager_total[$element];
  $i = $pager_first;
  if ($pager_last > $pager_max) {
    $i = $i + ($pager_max - $pager_last);
    $pager_last = $pager_max;
  }
  if ($i <= 0) {
    $pager_last = $pager_last + (1 - $i);
    $i = 1;
  }
  
  $default_tags = array(
    'first' => t('system', '« 首页'),
    'prev' => t('system', '‹ 上页'),
    'next' => t('system', '下页 ›'),
    'last' => t('system', '末页 »'),
    'sum' => t('system', '总记录：'),
    'count' => t('system', '页次：'),
    //'no_sum' => true, // 不显示总记录
    //'no_count' => true, // 不显示页次
    //'no_input' => true, // 不显示跳转输入框
  );
  
  $tags = array_merge($default_tags, $tags);
  
  $li_first = pager_view_first($tags['first'], $limit, $element, $parameters, $att);
  $li_previous = pager_view_previous($tags['prev'], $limit, $element, 1, $parameters, $att);
  $li_next = pager_view_next($tags['next'], $limit, $element, 1, $parameters, $att);
  $li_last = pager_view_last($tags['last'], $limit, $element, $parameters, $att);
  if ($pager_total[$element] > 1) {
    if (!$tags['no_sum']) {
      $items[] = array(
        'class' => 'pager_sum',
        'data' => $tags['sum'] . '<span class="pager_number">' .$pager_total_items[$element].'</span>',
      );
    }
    if (!$tags['no_count']) {
      $items[] = array(
        'class' => 'pager_count',
        'data' => $tags['count'] . '<span class="pager_number">' . $pager_current .'/' . $pager_max.'</span>',
      );
    }
    if ($li_first) {
      $items[] = array(
        'class' => 'pager_first',
        'data' => $li_first,
      );
    }
    if ($li_previous) {
      $items[] = array(
        'class' => 'pager_previous',
        'data' => $li_previous,
      );
    }
    
    if ($i != $pager_max) {
      if ($i > 1) {
        $items[] = array(
          'class' => 'pager_ellipsis',
          'data' => '…',
        );
      }
      for (; $i <= $pager_last && $i <= $pager_max; $i++) {
        if ($i < $pager_current) {
          $items[] = array(
            'class' => 'pager_item',
            'data' => pager_view_previous($i, $limit, $element, ($pager_current - $i), $parameters, $att),
          );
        }
        if ($i == $pager_current) {
          $items[] = array(
            'class' => 'pager_current',
            'data' => $i,
          );
        }
        if ($i > $pager_current) {
          $items[] = array(
            'class' => 'pager_item',
            'data' => pager_view_next($i, $limit, $element, ($i - $pager_current), $parameters, $att),
          );
        }
      }
      if ($i < $pager_max) {
        $items[] = array(
          'class' => 'pager_ellipsis',
          'data' => '…',
        );
      }
    }
    if ($li_next) {
      $items[] = array(
        'class' => 'pager_next',
        'data' => $li_next,
      );
    }
    if ($li_last) {
      $items[] = array(
        'class' => 'pager_last',
        'data' => $li_last,
      );
    }
    $key = $element + 1;
    
    if (!$tags['no_input']) {
      $items[] = array(
        'class' => 'pager_go_input',
        'data' => t('system', '转到第 !input 页', array('!input' => '<input type="text" value="'.floor($_GET['page']+1).'" alt="'.$key.'" class="form_text pager_form_go_input" id="pager_go_input_'.$key.'" /> ')),
      );
      $js[$key] = array('sum' => $pager_max, 'current' => $pager_page_array[$element]);
      dd_add_js(array('pager' => $js), 'setting');
    }
    
    return theme('item_list', $items, NULL, 'ul', array('class' => 'pager'));
  }
}

function pager_view_first($text, $limit, $element = 0, $parameters = array(), $attributes = array()) {
  global $pager_page_array;
  $output = '';
  if ($pager_page_array[$element] > 0) {
    $output = pager_view_link($text, pager_load_array(0, $element, $pager_page_array), $element, $parameters, $attributes);
  }
  return $output;
}

function pager_view_previous($text, $limit, $element = 0, $interval = 1, $parameters = array(), $attributes = array()) {
  global $pager_page_array;
  $output = '';

  if ($pager_page_array[$element] > 0) {
    $page_new = pager_load_array($pager_page_array[$element] - $interval, $element, $pager_page_array);

    if ($page_new[$element] == 0) {
      $output = pager_view_first($text, $limit, $element, $parameters, $attributes);
    } else {
      $output = pager_view_link($text, $page_new, $element, $parameters, $attributes);
    }
  }

  return $output;
}

function pager_view_next($text, $limit, $element = 0, $interval = 1, $parameters = array(), $attributes = array()) {
  global $pager_page_array, $pager_total;
  $output = '';
  if ($pager_page_array[$element] < ($pager_total[$element] - 1)) {
    $page_new = pager_load_array($pager_page_array[$element] + $interval, $element, $pager_page_array);
    if ($page_new[$element] == ($pager_total[$element] - 1)) {
      $output = pager_view_last($text, $limit, $element, $parameters, $attributes);
    } else {
      $output = pager_view_link($text, $page_new, $element, $parameters, $attributes);
    }
  }

  return $output;
}

function pager_view_last($text, $limit, $element = 0, $parameters = array(), $attributes = array()) {
  global $pager_page_array, $pager_total;
  $output = '';
  if ($pager_page_array[$element] < ($pager_total[$element] - 1)) {
    $output = pager_view_link($text, pager_load_array($pager_total[$element] - 1, $element, $pager_page_array), $element, $parameters, $attributes);
  }

  return $output;
}

function pager_view_link($text, $page_new, $element, $parameters = array(), $attributes = array()) {
  $page = isset($_GET['page']) ? $_GET['page'] : '';
  if ($new_page = implode(',', pager_load_array($page_new[$element], $element, explode(',', $page)))) {
    $parameters['page'] = $new_page;
  }

  $query = array();
  
  if (count($parameters)) {
    $query[] = dd_query_string_encode($parameters, array());
  }
  
  $querystring = pager_get_querystring();
  
  if ($querystring != '') {
    $query[] = $querystring;
  }

  if (!isset($attributes['title'])) {
    static $titles = NULL;
    if (!isset($titles)) {
      $titles = array(
        '« 首页' => '到第一页',
        '‹ 上页' => '到上一页',
        '下页 ›' => '到下一页',
        '末页 »' => '到最后一页',
      );
    }
    if (isset($titles[$text])) {
      $attributes['title'] = $titles[$text];
    } else if (is_numeric($text)) {
      $attributes['title'] = '转到第 '.$text .' 页';
    }
  }
  
  if (isset($attributes['fragment'])) {
    $fragment = $attributes['fragment'];
    unset($attributes['fragment']);
  } else {
    $fragment = NULL;
  }
  
  return l($text, $_GET['q'], array(
    'fragment' => $fragment,
    'attributes' => $attributes,
    'query' => count($query) ? implode('&', $query) : NULL)
  );
  
}

function pager_load_array($value, $element, $old_array) {
  $new_array = $old_array;
  for ($i = 0; $i < $element; $i++) {
    if (!$new_array[$i]) {
      $new_array[$i] = 0;
    }
  }
  
  $new_array[$element] = (int)$value;
  return $new_array;
}
