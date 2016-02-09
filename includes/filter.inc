<?php
// $Id$

/**
 * 格式化内容
 * @param (string) $text
 *  原始字符串
 * @param (int) $format
 *  输入格式 id
 * @param (string) $type
 *  自定义类型，一般用于缓存
 * @param (int) $id
 *  自定义 id，一般用于缓存
 */
function filter_view($text, $format = NULL, $type = NULL, $id = NULL) {
  global $conf;

  if (!isset($format) || !$conf['formats'][$format]) {
    $format = $conf['default_format'];
  }
  
  $options = filter_user_format();
  
  // 是否缓存内容，当内容不发生变更，下次请求将直接从数据库读取缓存。这将减少 PHP 开销，却会增加数据库查询
  if ($conf['filter_cache']) {
  
    // 含有 php 过滤器，则不缓存
    
    if ($cache = !filter_is_php($format)) {
      $cache_id = $format .':'. md5($text);
      if ($cached = cache_get($cache_id, 'cache_filter')) {
        
        // 代码高亮，加载 js
        if (filter_is_filter($format, 'editor', 'highlighter')) {
          editor_get_highlighter();
        }
        
        return $cached->data;
      }
    }
    
  } else {
    $cache = false;
  }
  
  if ($conf['filters'][$format] && is_array($conf['filters'][$format])) {
    foreach ($conf['filters'][$format] as $filter) {
      $text = call_user_func($filter['module'] . '_filter', 'view', $filter['delta'], $filter['format'], $text, $filter['filter'], $type, $id);
    }
  }
  
  if ($cache) {
    cache_set($cache_id, $text, 'cache_filter');
  }
  
  return $text;
}

/**
 * PHP 解析
 */
function filter_tags_php($text) {
  // 判断全局是否启用 php code
  if ($GLOBALS['_filter_is_php_code'] == true) {
    if (strpos($text, '<?php') !== false) {
      return preg_replace_callback('/<\?php(.*?)\?>/ms', '_filter_tags_php', $text);
    }
  }
  return $text;
}

function _filter_tags_php($matches) {
  if (!empty($matches[1])) {
    ob_start();
    eval($matches[1]);
    $text = ob_get_contents();
    ob_end_clean();
    return $text;
  }
}

/**
 * 获取当前用户输入格式
 */
function filter_user_format($ac = NULL) {
  global $conf;
  $options = array();
  
  if ($conf['formats']) {
    if (!$ac) {
      $ac = $GLOBALS['user'];
    }
    if ($ac->uid != 1) {
      $fids = array();
      if ($conf['default_format']) {
        $fids[] = $conf['default_format'];
      }
      if ($ac->roles) {
        foreach ($ac->roles as $rid => $c) {
          if (is_array($conf['format_roles'][$rid])) {
            $fids = array_merge($fids, $conf['format_roles'][$rid]);
          }
        }
      }
      if ($fids) {
        foreach ($conf['formats'] as $f) {
          if (in_array($f['format'], $fids)) {
            $options[$f['format']] = $f['name'];
            if  (!empty($f['description'])) {
              $options[$f['format']] .= '<span class="description form_description form_description_format">' . check_plain(t('system', $f['description'])) . '</span>';
            }
          }
        }
      }
    } else {
      foreach ($conf['formats'] as $f) {
        $options[$f['format']] = $f['name'];
        if  (!empty($f['description'])) {
          $options[$f['format']] .= '<span class="description form_description form_description_format">' . check_plain(t('system', $f['description'])) . '</span>';
        }
      }
    }
  }
  return $options;
}

/**
 * 输入格式
 */
function filter_form_field($format = NULL, $required = false, $title = NULL) {
  global $user, $conf;

  if (!isset($format) || !$conf['formats'][$format]) {
    $format = $conf['default_format'];
  }

  $option = filter_user_format();
  if (count($option) > 1) {
    $field = array(
      '#fieldset_prefix' => 'desc',
      '#fieldset_legend' => $title ? $title : t('system', '输入格式'),
      '#type' => 'radio',
      '#required' => $required,
      '#default_value' => $format,
      '#fieldset_suffix' => 1,
      '#options' => filter_user_format()
    );
  } else {
    $field = array(
      '#type' => 'hidden',
      '#default_value' => $format,
      '#constant' => 1
    );
  }
  return $field;
}

/**
 * 输入格式、角色格式配置缓存
 */
function filter_set_cache() {
  if ($fetch = db_query('SELECT roles, format, name, description FROM {filter_format}', NULL, array('fetch' => 'array'))) {
    $roles = array();
    foreach ($fetch as $o) {
      if ($o['roles']) {
        $o['roles'] = unserialize($o['roles']);
        if (is_array($o['roles'])) {
          foreach ($o['roles'] as $rid) {
            $roles[$rid][$o['format']] = $o['format'];
          }
        }
      }
      $formats[$o['format']] = $o;
    }
  }
  
  if ($fetch = db_query('SELECT filter, format, delta, module FROM {filters} ORDER BY weight ASC, fid DESC', NULL, array('fetch' => 'array'))) {
    foreach ($fetch as $o) {
      $filters[$o['format']][] = $o;
    }
  }
  
  cache_del('cid', 'filter_get_filters');
  var_set('formats', $formats, 0);
  var_set('filters', $filters, 0);
  var_set('format_roles', $roles, 0);
  cache_del('empty', NULL, 'cache_filter');
  
  var_init();
}

/**
 * 检查输入法中是否含有 php 过滤器
 */
function filter_is_php($format) {
  return filter_is_filter($format, 'system', 'php');
}

/**
 * 
 * 检查输入法中是否含有某过滤器
 * @param (int) $format
 *  输入法
 * @param (string) $module
 *  模块名称
 * @param (*) $delta
 *  过滤器 delta
 */
function filter_is_filter($format, $module, $delta) {
  global $conf;
  if ($conf['filters'] && $conf['filters'][$format]) {
    foreach ($conf['filters'][$format] as $f) {
      if ($f['module'] == $module && $f['delta'] == $delta) {
        return true;
      }
    }
  }
  
  return false;
}

/**
 * 获取自定义过滤器
 */
function filter_get_filters($fid = NULL) {
  static $filters;
  
  if (!isset($filters)) {
    if ($cache = cache_get('filter_get_filters')) {
      $filters = $cache->data;
    } else {
      if ($fetch = db_query('SELECT * FROM {filter_tags}')) {
        foreach ($fetch as $o) {
          if ($o->module == 'system' && $o->body) {
            switch ($o->delta) {
              case 'html_enabled': case 'html_disabled':
                if ($tags = explode(' ', $o->body)) {
                  foreach ($tags as $tag) {
                    $o->tags[$tag] = 1;
                    $o->strip_tags .= '<'.$tag.'>';
                  }
                }
              break;
              case 'keyword':
                if ($tags = dd_line_to_array($o->body, true)) {
                  foreach ($tags as $tag) {
                    if (strpos($tag, '|') === false) {
                      $o->tags[$tag] = "";
                    } else {
                      $t = explode('|', $tag, 2);
                      $o->tags[$t[0]] = $t[1];
                    }
                  }
                }
              }
          }
          $filters[$o->fid] = $o;
        }
      }
      cache_set('filter_get_filters', $filters);
    }
  }
  
  return $fid ? $filters[$fid] : $filters;
}

// public filter_string_to_pinyin($string, $first = false) {{{ 
/**
 * filter_string_to_pinyin
 *  汉字转为拼音
 * 
 * @param string $string 
 *  待转换的汉字
 * @param bool $first 
 *  只返回首字母
 * @access public
 * @return string
 */
function filter_string_to_pinyin($string, $first = false){
  $_dataKey = "a|ai|an|ang|ao|ba|bai|ban|bang|bao|bei|ben|beng|bi|bian|biao|bie|bin|bing|bo|bu|ca|cai|can|cang|cao|ce|ceng|cha". 
    "|chai|chan|chang|chao|che|chen|cheng|chi|chong|chou|chu|chuai|chuan|chuang|chui|chun|chuo|ci|cong|cou|cu|". 
    "cuan|cui|cun|cuo|da|dai|dan|dang|dao|de|deng|di|dian|diao|die|ding|diu|dong|dou|du|duan|dui|dun|duo|e|en|er". 
    "|fa|fan|fang|fei|fen|feng|fo|fou|fu|ga|gai|gan|gang|gao|ge|gei|gen|geng|gong|gou|gu|gua|guai|guan|guang|gui". 
    "|gun|guo|ha|hai|han|hang|hao|he|hei|hen|heng|hong|hou|hu|hua|huai|huan|huang|hui|hun|huo|ji|jia|jian|jiang". 
    "|jiao|jie|jin|jing|jiong|jiu|ju|juan|jue|jun|ka|kai|kan|kang|kao|ke|ken|keng|kong|kou|ku|kua|kuai|kuan|kuang". 
    "|kui|kun|kuo|la|lai|lan|lang|lao|le|lei|leng|li|lia|lian|liang|liao|lie|lin|ling|liu|long|lou|lu|lv|luan|lue". 
    "|lun|luo|ma|mai|man|mang|mao|me|mei|men|meng|mi|mian|miao|mie|min|ming|miu|mo|mou|mu|na|nai|nan|nang|nao|ne". 
    "|nei|nen|neng|ni|nian|niang|niao|nie|nin|ning|niu|nong|nu|nv|nuan|nue|nuo|o|ou|pa|pai|pan|pang|pao|pei|pen". 
    "|peng|pi|pian|piao|pie|pin|ping|po|pu|qi|qia|qian|qiang|qiao|qie|qin|qing|qiong|qiu|qu|quan|que|qun|ran|rang". 
    "|rao|re|ren|reng|ri|rong|rou|ru|ruan|rui|run|ruo|sa|sai|san|sang|sao|se|sen|seng|sha|shai|shan|shang|shao|". 
    "she|shen|sheng|shi|shou|shu|shua|shuai|shuan|shuang|shui|shun|shuo|si|song|sou|su|suan|sui|sun|suo|ta|tai|". 
    "tan|tang|tao|te|teng|ti|tian|tiao|tie|ting|tong|tou|tu|tuan|tui|tun|tuo|wa|wai|wan|wang|wei|wen|weng|wo|wu". 
    "|xi|xia|xian|xiang|xiao|xie|xin|xing|xiong|xiu|xu|xuan|xue|xun|ya|yan|yang|yao|ye|yi|yin|ying|yo|yong|you". 
    "|yu|yuan|yue|yun|za|zai|zan|zang|zao|ze|zei|zen|zeng|zha|zhai|zhan|zhang|zhao|zhe|zhen|zheng|zhi|zhong|". 
    "zhou|zhu|zhua|zhuai|zhuan|zhuang|zhui|zhun|zhuo|zi|zong|zou|zu|zuan|zui|zun|zuo"; 

  $_dataValue = "-20319|-20317|-20304|-20295|-20292|-20283|-20265|-20257|-20242|-20230|-20051|-20036|-20032|-20026|-20002|-19990". 
    "|-19986|-19982|-19976|-19805|-19784|-19775|-19774|-19763|-19756|-19751|-19746|-19741|-19739|-19728|-19725". 
    "|-19715|-19540|-19531|-19525|-19515|-19500|-19484|-19479|-19467|-19289|-19288|-19281|-19275|-19270|-19263". 
    "|-19261|-19249|-19243|-19242|-19238|-19235|-19227|-19224|-19218|-19212|-19038|-19023|-19018|-19006|-19003". 
    "|-18996|-18977|-18961|-18952|-18783|-18774|-18773|-18763|-18756|-18741|-18735|-18731|-18722|-18710|-18697". 
    "|-18696|-18526|-18518|-18501|-18490|-18478|-18463|-18448|-18447|-18446|-18239|-18237|-18231|-18220|-18211". 
    "|-18201|-18184|-18183|-18181|-18012|-17997|-17988|-17970|-17964|-17961|-17950|-17947|-17931|-17928|-17922". 
    "|-17759|-17752|-17733|-17730|-17721|-17703|-17701|-17697|-17692|-17683|-17676|-17496|-17487|-17482|-17468". 
    "|-17454|-17433|-17427|-17417|-17202|-17185|-16983|-16970|-16942|-16915|-16733|-16708|-16706|-16689|-16664". 
    "|-16657|-16647|-16474|-16470|-16465|-16459|-16452|-16448|-16433|-16429|-16427|-16423|-16419|-16412|-16407". 
    "|-16403|-16401|-16393|-16220|-16216|-16212|-16205|-16202|-16187|-16180|-16171|-16169|-16158|-16155|-15959". 
    "|-15958|-15944|-15933|-15920|-15915|-15903|-15889|-15878|-15707|-15701|-15681|-15667|-15661|-15659|-15652". 
    "|-15640|-15631|-15625|-15454|-15448|-15436|-15435|-15419|-15416|-15408|-15394|-15385|-15377|-15375|-15369". 
    "|-15363|-15362|-15183|-15180|-15165|-15158|-15153|-15150|-15149|-15144|-15143|-15141|-15140|-15139|-15128". 
    "|-15121|-15119|-15117|-15110|-15109|-14941|-14937|-14933|-14930|-14929|-14928|-14926|-14922|-14921|-14914". 
    "|-14908|-14902|-14894|-14889|-14882|-14873|-14871|-14857|-14678|-14674|-14670|-14668|-14663|-14654|-14645". 
    "|-14630|-14594|-14429|-14407|-14399|-14384|-14379|-14368|-14355|-14353|-14345|-14170|-14159|-14151|-14149". 
    "|-14145|-14140|-14137|-14135|-14125|-14123|-14122|-14112|-14109|-14099|-14097|-14094|-14092|-14090|-14087". 
    "|-14083|-13917|-13914|-13910|-13907|-13906|-13905|-13896|-13894|-13878|-13870|-13859|-13847|-13831|-13658". 
    "|-13611|-13601|-13406|-13404|-13400|-13398|-13395|-13391|-13387|-13383|-13367|-13359|-13356|-13343|-13340". 
    "|-13329|-13326|-13318|-13147|-13138|-13120|-13107|-13096|-13095|-13091|-13076|-13068|-13063|-13060|-12888". 
    "|-12875|-12871|-12860|-12858|-12852|-12849|-12838|-12831|-12829|-12812|-12802|-12607|-12597|-12594|-12585". 
    "|-12556|-12359|-12346|-12320|-12300|-12120|-12099|-12089|-12074|-12067|-12058|-12039|-11867|-11861|-11847". 
    "|-11831|-11798|-11781|-11604|-11589|-11536|-11358|-11340|-11339|-11324|-11303|-11097|-11077|-11067|-11055". 
    "|-11052|-11045|-11041|-11038|-11024|-11020|-11019|-11018|-11014|-10838|-10832|-10815|-10800|-10790|-10780". 
    "|-10764|-10587|-10544|-10533|-10519|-10331|-10329|-10328|-10322|-10315|-10309|-10307|-10296|-10281|-10274". 
    "|-10270|-10262|-10260|-10256|-10254"; 

  $_tdataKey   = explode('|', $_dataKey); 
  $_tdataValue = explode('|', $_dataValue);
  $_data = array_combine($_tdataKey, $_tdataValue);
  arsort($_data); 
  reset($_data);

  $_c = $string;
  $string = ''; 

  if ($_c < 0x80) {
    $string .= $_c;
  } else if ($_c < 0x800) { 
    $string .= chr(0xC0 | $_c >> 6); 
    $string .= chr(0x80 | $_c & 0x3F); 
  } else if ($_c < 0x10000) { 
    $string .= chr(0xE0 | $_c >> 12); 
    $string .= chr(0x80 | $_c >> 6 & 0x3F); 
    $string .= chr(0x80 | $_c & 0x3F); 
  } else if ($_c < 0x200000) { 
    $string .= chr(0xF0 | $_c>>18); 
    $string .= chr(0x80 | $_c>>12 & 0x3F); 
    $string .= chr(0x80 | $_c>>6 & 0x3F); 
    $string .= chr(0x80 | $_c & 0x3F); 
  }

  $string = iconv('UTF-8', 'GB2312', $string); 

  $_res = ''; 

  for ($i=0; $i < strlen($string); $i++) { 
    $_p = ord(substr($string, $i, 1)); 
    if ($_p>160) { 
      $_q = ord(substr($string, ++$i, 1));
      $_p = $_p*256 + $_q - 65536;
    } 
    $_res .= _filter_string_to_pinyin($_p, $_data); 
  } 
 
  $output = preg_replace("/[^a-z0-9]*/", '', $_res);

  return $first ? substr($output, 0, 1) : $output;
}
// }}}

function _filter_string_to_pinyin($num, $data) { 
  if ($num > 0 && $num < 160 ) {
    return chr($num);
  } else if ($num < -20319 || $num > -10247) {
    return '';
  } else { 
    foreach($data as $k => $v) { 
      if ($v <= $num) break; 
    } 
    return $k; 
  } 
}


