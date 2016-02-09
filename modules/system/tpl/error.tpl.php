<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
  <title><?php echo dd_get_title(); ?></title>
  <?php echo dd_get_html_head(); ?>
  <?php echo dd_get_css(); ?>
  <?php echo dd_get_js('header'); ?>
</head>
<body id="print">
  <?php echo $content; ?>
  <?php echo dd_get_js('footer'); ?>
</body>
</html>