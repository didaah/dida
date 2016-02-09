<?php
// $Id$

/**
 * 生成验证码页面，可输出文本和图片两种格式
 * @access public
 * @return void
 */
function captcha_page() {
  if (!empty($_GET['data'])) {
    $value = unserialize(dd_decrypt($_GET['data']));
  } else if (!empty($_POST['data'])) {
    $value = unserialize(dd_decrypt($_POST['data']));
  } else {
    echo 'ERROR!';
    exit;
  }

  if (!empty($value['name']) && !empty($value['form_id'])) {

    $str = _captcha_get($value);

    header('Cache-Control: no-cache, must-revalidate');

    header('Expires: Sat, 26 Jul 1997 05:00:00 GMT');
    
    if ($value['image']) {
      if ($ttf = var_get('captcha_file_ttf')) {
        $ttf = DIDA_ROOT . $ttf;
        if (is_file($ttf)) {
          header('Content-type: image/png');

          $size = imageftbbox(12, 0, $ttf, $str, array('linespacing' => 1));
          
          $height = abs($size[1])+abs($size[5])+10;
          $width = abs($size[0])+abs($size[2])+20;
          
          $im = @imagecreate($width, $height);
          imagecolorallocate($im, 255, 255, 255);
          $black = imagecolorallocate($im, 233, 53, 91);

          // 加入干扰码
          if (!empty($value['mosaic'])) {
            for ($i = 0; $i < $width; ++$i) {
              $rand = rand(0, 2);
              $i += $rand;
              $color = imagecolorallocate($im, rand(190, 250), rand(190, 250), rand(190, 250));
              imageline($im, $rand, $i, $width, $i, $color);
              imageline($im, $i, $rand, $i, $height, $color);
            }
          }

          imagefttext($im, 12, 0, 10, $height-5, $black, $ttf, $str, array('linespacing' => 1));

          imagepng($im);
          imagedestroy($im);
          exit;
        }
      }
    }
    
    header('Content-Type: text/plain; charset=utf-8');
    print $str;
    exit;
  }

  exit;
}

//function captcha_get($values$name, $form_id, $form_token, $type, $mosaic, $image = 0) {
/**
 * 获取验证码 
 * @param array $values 
 *  验证码设置，示例：
 *    array(
 *      'name' => 'captcha', // 验证码表单字段名
 *      'form_id' => 'my_form', // 表单 id
 *      'tag' => '', // 必须是表单 token，若不对应，将无法通过验证
 *      'type' => 1, // 验证码类型
 *      'mosaic' => 1, // 是否添加干扰码
 *      'image' => true, // 是否图形验证码，false 则为文本
 *    )
 * @access public
 * @return string
 */
function captcha_get(array $values) {
  if (empty($values['image'])) {
    return _captcha_get($values);
  } else {
    return '<img class="form_captcha_img" src="' . url('captcha', array('query' => array(
      'data' => dd_encrypt(serialize($values)),
      'timestamp' => 1
    ))) . '" />';
  }
}

/**
 * 获取验证码字符串
 * @param array $values 
 *  验证码参数
 * @access protected
 * @return string
 */
function _captcha_get(array $values) {
  switch ($values['type']) {
    case 1:
      $array1 = range(1, 199);
      $array2 = range('a', 'z');
      $a = array_rand($array1, 2);
      $b = array_rand($array2, 2);
      $cap = $array1[$a[0]] . $array2[$b[0]] . $array1[$a[1]]  . $array2[$b[1]];
      $str = $cap;
    break;
    case 2:
      $cap = _captcha_type_custom();
      $str = $cap;
    break;
    case 3:
      $a = rand(1, 10);
      $b = rand(1, 10);
      $c = rand(1, 10);
      $d = rand(1, 10);
      $str = t('system', '请计算：!string', array('!string' => "{$a}+{$b}x{$c}-{$d} = ?"));
      $cap = $a + $b * $c - $d;
    break;
    case 4:
      $array = _captcha_type_ask();
      $cap = $array[1];
      $str = t('system', '请回答：!string', array('!string' => $array[0]));
    break;
    default:
      $a = rand(1, 10);
      $b = rand(1, 10);
      $c = rand(1, 10);
      $d = rand(1, 10);
      $cap = $a. $b. $c. $d . $d. $c;
      $cap = substr($cap, 0, 4);
      $str = $cap;
  }

  $_SESSION['captcha'][$values['tag']. $values['form_id'] . $values['name']] = $cap;
  
  return $str;
}

/**
 * 自定义文本 
 * @access protected
 * @return void
 */
function _captcha_type_custom() {
  if (!$data = custom_get('captcha_data_custom')) {
    $data = array(
      '西当太白有鸟道',
      '可以横绝峨眉巅',
      '上有六龙回日之高标',
      '下有冲波逆折之回川',
      '地崩山摧壮士死',
      '然后天梯石栈相钩连',
      '问君西游何时还',
      '连峰去天不盈尺',
      '枯松倒挂倚绝壁',
      '难于上青天',
      '嗟尔远道之人胡为乎来哉'
    );
  }

  return $data[array_rand($data)];
}

/**
 * 问答式验证码 
 * @access protected
 * @return void
 */
function _captcha_type_ask() {
  if (!$data = custom_get('captcha_data_ask')) {
    $data = array(
      array('世界上含沙量最大的河是？', '黄河'),
      array('世界上最早的字典？', '说文解字'),
      array('世界上最大的石刻佛像？', '乐山大佛'),
      array('世界上最高的山峰？', '珠穆朗玛峰')
    );
  }

  return $data[array_rand($data)];
}
