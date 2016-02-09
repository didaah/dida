// $Id$

jQuery(function($) {
  var s = Dida.getsize();
  if (s[1] > $('body').height()) {
    $('#footer').css({'position': 'fixed', '_position': 'absolute', 'bottom': '0px', 'left': '0px'});
  }
  $('input:hidden').next('label.error').hide();
});

