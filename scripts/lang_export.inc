<?php
// $Id$

/**
 * @file
 *  按模块生成语言翻译文件
 *  终端命令示例：
 *    为 www.didaah.org 站点下 system 模块生成英文语言包
 *    翻译语言文件将保存为 {module_name}/languages/{lang}.php，已有翻译语言文件将被覆盖，但原有翻译字符串将被保留
 *    生成 system 模块的英文语言文件
 *     php scripts/common.inc --host www.didaah.org --bootstrap full --script_name lang_export.inc --module_name system --lang en
 *    生成所有已启用模块的英文语言文件
 *     php scripts/common.inc --host www.didaah.org --bootstrap full --script_name lang_export.inc --lang en
 *    生成所有已启用模块的英文语言文件，并导入已有的翻译（包括已合并的翻译文件和自定义翻译文件）
 *     php scripts/common.inc --host www.didaah.org --bootstrap full --script_name lang_export.inc --lang en --export_value 1
 *    生成所有已启用模块的英文语言文件，如语言文件已存在，直接替换
 *     php scripts/common.inc --host www.didaah.org --bootstrap full --script_name lang_export.inc --clear_cache 1 --lang en
 *    
 *    @param
 *      --lang 指定语言，如 en，默认空
 *      --clear_cache 1 直接替换已有文件，0 备份已有文件，默认 0
 *      --export_value 1 导出已有翻译值，默认为 0
 *      --is_new 1 忽略已有文件中的待翻译列表，以当前解析的待翻译列表为准 默认 0
 *
 * @TODO js 中的多语言待确定
 */

require_once DIDA_ROOT . '/modules/system/system.admin.inc';

if (!empty($_args['lang'])) {
  if (!empty($_args['module_name'])) {
    _lang_export($_args['lang'], $_args['module_name']);
  } else if ($fetch = db_query('SELECT * FROM {system}')) {
    foreach ($fetch as $o) {
      _lang_export($_args['lang'], $o->filename);
    }
  }
} else {
  message('请指定等导出的语言');
}

function _lang_export($lang_code, $module) {
  global $_args, $conf_dir;

  if (!empty($_args['export_value'])) {
    /**
     * 读取该语言所有已翻译的字符串，做为默认翻译
     */
    $cache_lang_file = DIDA_ROOT . '/' . $conf_dir . '/cache/languages/';
    $cache_lang_value = $cache_lang_js = array();

    if (is_file($cache_lang_file . $lang_code . '.php')) {
      // 已合并的缓存翻译文件
      include $cache_lang_file . $lang_code . '.php';
      $cache_lang_value = $l;
      $cache_lang_js = $j;
      // 加载自定义翻译文件
      if (is_file($cache_lang_file . 'custom_' . $lang_code . '.php')) {
        $l = $j = array();
        include $cache_lang_file . 'custom_' . $lang_code . '.php';
        $cache_lang_value = array_merge($cache_lang_value, $l);
        if (!empty($j)) {
          $cache_lang_js = array_merge($cache_lang_js, $j);
        }
      }

      if (!empty($cache_lang_js)) {
        foreach ($cache_lang_value as $key => $value) {
          if ($key == $value && !empty($cache_lang_js[$key]) && $cache_lang_js[$key] != $key) {
            $cache_lang_value[$key] = $cache_lang_js[$key];
          } else if ($key != $value && !empty($cache_lang_js[$key]) && $cache_lang_js[$key] == $key) {
            $cache_lang_js[$key] = $value;
          }
        }
      }

    }
  }

  if (!$lang_name = system_get_languages_data($lang_code)) {
    message('语言不存在');
  } else if (!$path = db_query('SELECT path FROM {system} WHERE filename = ?', array($module), array('return' => 'column'))) {
    echo $module;
    message('模块或主题不存在');
  } else {
    
    message('正在导出模块 - ' . $module);

    message('正在导出语言 - ' . $lang_name . '(' . $lang_code . ')');
     
    $path = DIDA_ROOT . '/' . $path;
    
    $files = _lang_export_files($path);

    // system 模块，加上系统核心文件
    if ($module == 'system') {
      $files = _lang_export_files(DIDA_ROOT . '/includes', $files);
      $files = _lang_export_files(DIDA_ROOT . '/misc', $files);
    }

    if (!empty($files)) {
      
      include_once $path . '/' . $module . '.info';

      // 截入已有翻译文件
      
      $lang_file = $path . '/languages';

      if (!is_dir($lang_file) && !mkdir($lang_file, 0777)) {
        message($lang_file . '创建失败，无法自动保存翻译文件');
      }

      $lang_file .=  '/' . $lang_code . '.php'; 
      
      // 所有待翻译的界面字符串
      $lang = array();

      if (empty($_args['is_new']) && is_file($lang_file)) {
        include_once $lang_file;
      }

      // js 文件中使用到界面字符串
      $js_lang = array();

      // 模块界面默认语言，若无，则为中文简体
      $default_lang = !empty($info['language']) ? $info['language'] : 'zh-hans';

      foreach ($files as $file) {
        message('正在分析文件 - ' . $file);
        if ($string = file_get_contents($file)) {

          $mat = $mat2 = array();

          $string = str_replace(array('\\"', "\\'"), array('[&__a]', '[&__b]'), $string);
          
          $filetype = substr($file, -2);

          if ($filetype != 'js') {
            // php 文件中使用 t() 函数实现多语言
            preg_match_all('/[^\'|"]t\(.{1,4}' . $module . '[\'|"].{1,3}\'(.*?)\'/ms', $string, $mat); 
            preg_match_all('/[^\'|"]t\(.{1,4}' . $module . '[\'|"].{1,3}"(.*?)"/ms', $string, $mat2);
          } else {
            // js 文件中使用 Dida.t() 实现，使用上面的正则也能匹配，但为防止冲突，使用更为精准的匹配
            preg_match_all('/[^\'|"]Dida\.t\(.{1,4}' . $module . '[\'|"].{1,3}\'(.*?)\'/ms', $string, $mat); 
            preg_match_all('/[^\'|"]Dida\.t\(.{1,4}' . $module . '[\'|"].{1,3}"(.*?)"/ms', $string, $mat2);
          }
          
          if (!empty($mat2[1])) {
            $mat[1] = array_merge($mat[1], $mat2[1]);
          }

          foreach ($mat[1] as $value) {
            if ($filetype == 'js') {
              $js_lang[$value] = 1;
            }
            if (!empty($_args['export_value']) && !empty($cache_lang_value[$value])) {
              $lang[$value] = $cache_lang_value[$value];
            } else if (empty($lang[$value])) {
              $value = str_replace(array('[&__a]', '[&__b]'), array('"', "'"), $value);
              $lang[$value] = $value;
            }
          }
        }
      }
      
      if (!empty($lang)) {
        $output = '<?php';
        $output .= "\n";
        $output .= '// $';
        $output .= 'Id';
        $output .= '$';

        $message = "\n\n/**\n";
        $message .= ' * 默认语言：' . system_get_languages_data($default_lang) . "($default_lang)\n";
        $message .= ' * 翻译语言：' . $lang_name . '(' . $lang_code . ")\n";
        $message .= ' * 共有 ' . count($lang) . " 条翻译\n";
        $message .= " */\n\n";

        $output .= $message;
        $output .= '$lang = ' . var_export($lang, true) . ";\n";
        $output .= '$js_lang = ' . var_export($js_lang, true) . ";\n"; 
        
        message($message);

        // 已有同名语言文件存在，若未传递清除参数，将该文件保存为备份，以免替换该文件造成损失
        if (empty($_args['clear_cache']) && is_file($lang_file)) {
          rename($lang_file, $lang_file . '.bak');
        }

        if (file_put_contents($lang_file, $output)) {
          message('已保存至 ' . $lang_file);
        } else {
          message('翻译文件未成功保存至 ' . $lang_file);
        }
      } else {
        message('没有需要翻译的字符串');
      }
      
    } else {
      message('没有需要翻译的字符串');
    }
  }
}

/**
 * 获取指定目录下所有待翻译的文件 
 * @param string $dir 
 *  目录
 * @param array $files 
 * @access protected
 * @return array
 */
function _lang_export_files($dir, &$files = array()) {

  if (is_dir($dir)) {
    if ($objs = glob($dir.'/*')) {
      foreach ($objs as $obj) {
        if (is_dir($obj)) {
          _lang_export_files($obj, $files);
        } else if (in_array(end(explode('.', $obj)), array('module', 'install', 'info', 'inc', 'php', 'js'))) {
          $files[] = $obj;
        }
      }
    }
  }

  return $files;
}
