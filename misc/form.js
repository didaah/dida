// $Id$
$(function() {
  $("input[type='text'], input[type='password']").hover(function() {
    $(this).addClass('inputhover');
  },function() {
    $(this).removeClass('inputhover');
  })

  $('.form_captcha').click(function() {
    if (Dida.settings.captcha) {
      var k = $(this).attr('alt');
      var opt = Dida.settings.captcha[k];
      var $$ = $(this);
      
      opt.timestamp = Dida.gettime();

      if (opt.image) {
        $$.html('<img class="form_captcha_img" src="'+ Dida.url('captcha', opt) +'" />');
      } else {
        $.post(Dida.settings.base_path + 'captcha', opt, function(data) {
          $$.html(data);
        });
      }
    }
  });

  $('a.form_file_click').click(function() {
    length = $(this).prev().attr('multi');
    dom = $(this).attr('alt');
    sum = $('input', '#' + dom).length;
    if (sum != length) {
      $($($(this).prev()).clone()).insertAfter($(this).parent());
      $('#' + dom).children('input').not($(this).prev()).wrapAll('<div></div>');
      document.cookie = dom + "=" + (sum+1);
      if (sum == (length - 1)) {
         $(this).remove();
      }
    } else {
      $(this).remove();
      alert(Dida.t('system', '最多允许同时上传 %length 个文件', {'%length': length}));
    }
    return false;
  });
  
  $('input#user_login').keypress(function(event) {
  	if (event.which == 0) {
  		$('input#user_login_form_type_pass').focus();
  		return false;
  	}
  });

  $('span.option_label, span.form_radio_text').click(function() {
    if (!$(this).prev('input').attr('disabled')) {
  	  $(this).prev('input').attr('checked', !$(this).prev('input').attr('checked'));
    }
  });

  $('span.form_description').click(function() {
  	$(this).prev('input').focus();
  });

  $('label.dd_label').click(function() {
  	$(this).next().focus();
  });
});
