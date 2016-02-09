// $Id$
jQuery(function($) {
  if ($.fn.button.noConflict) {
    $.fn.btn = $.fn.button.noConflict();
  }

  $.fn.modal.Constructor.prototype.enforceFocus = function () {};
   
  $('.modal-link').live('click', function() {
    eval('var data = ' + ($(this).attr('source') || '{}') + ';');
    Dida.modal(data);
    return false;
  });

  $('.dida-modal-wrapper').live('hidden', function() {
    $(this).find('.btn-primary').off('click');
    $(this).find('.btn-cancel').off('click');
  });

  $('ul.site_menu_children').addClass('dropdown-menu');
  $('ul.site_menu > li > ul').each(function() {
    $(this).parent().addClass('dropdown');
    $(this).prev().addClass('dropdown-toggle').append('<b class="caret"></b>').attr('data-toggle', 'dropdown');
  });

  $('ul.site_menu_children > li > ul').each(function() {
    $(this).parent().addClass('dropdown-submenu');
  });

  $('ul.site_menu a.active').parent('li').addClass('active');
  $('ul.nav-tabs a.active, ul.nav-pills a.active').parent('li').addClass('active');
  $('ul.site_menu_children a.active').parents('.site_menu_list').addClass('active');
  $('.dropdown .item_dd_item_lang_change').addClass('dropdown-menu');

  $('#block_system_adminLink .item_admin_menus > h3').prepend('<i class="icon-plus"></i>');
});

Dida.modal = function(opt, callback) {
  var dom = '#'+(opt.dom || 'modal-wrapper');
  var button = '<button type="button" class="btn btn-default btn-cancel" data-dismiss="modal">'+Dida.t('system', '取消')+'</button>';
      button += '<button class="btn active btn-primary" type="button">'+Dida.t('system', '确认')+'</button>';

  $(dom).remove();
  var html = '';
  html = '<div class="modal fade dida-modal-wrapper" id="'+dom.substr(1)+'" aria-hidden="true" role="dialog">';
  html += '<div class="modal-dialog">';
  html += '<div class="modal-content">';
  html += '<div class="modal-header">';
  html += '<h3></h3>';
  html += '</div>';
  html += '<div class="modal-body"></div>';
  html += '<div class="modal-footer">';
  html += '<div class="button-wrapper">';
  html += button;
  html += '</div>';
  html += '</div>';
  html += '</div>';
  html += '</div>';
  html += '</div>';
  $('body').append(html);

  $(dom+' .button-wrapper').html(button);

  var data = {ok: Dida.t('system', '确认'), cancel: Dida.t('system', '取消'), options: {backdrop: 'static'}, title: 'Message'}

  $.extend(data, opt);

  if (data.content) {
    if (typeof data.content == 'string') {
      data.content = data.content.replace(/(\[\[|\]\])/g, function($0, $1) {
        if ($1 == '[[') {
          return '<';
        } else {
          return '>';
        }
      });
    }
    $(dom+' .modal-body').html(data.content);
  }

  if (opt.width) {
    $(dom).css({width: opt.width+'px'});
  }
  
  if (opt.height) {
    $(dom+' .modal-body').css({height: opt.height+'px'});
  }

  var s = Dida.getsize();
  var h = $(dom).height()+20;
  if (s[1] > h) {
    $(dom).css('top', ((s[1]-h)/2)+'px');
  } else {
    var h = s[1]-parseInt($(dom+' .modal-header').innerHeight())-parseInt($(dom+' .modal-footer').innerHeight())-50;
    $(dom+' .modal-body').css({height: h+'px', 'overflow-y': 'auto'});
    $(dom).css('top', '2px');
  }

  if (data.title) {
    $(dom+' .modal-header h3').html(data.title);
  }

  if (data.callback && $.isFunction(data.callback)) {
    $(dom+' .btn-primary').on('click', function() {
      data.callback($(dom));
      return false;
    });
  }

  if (data.close && $.isFunction(data.close)) {
    $(dom+' .btn-cancel').on('click', function() {
      data.close($(dom));
    });
  }

  $(dom+' .btn-primary').text(data.ok);
  $(dom+' .btn-cancel').text(data.cancel);

  if (data.hidden_ok) {
    $(dom+' .btn-primary').hide();
  } else {
    $(dom+' .btn-primary').show();
  }

  if (data.hidden_cancel) {
    $(dom+' .btn-cancel').hide();
    $(dom+' li.next').removeClass('next').addClass('ok');
  } else {
    $(dom+' .btn-cancel').show();
    $(dom+' li.ok').addClass('next').removeClass('ok');
  }

  if (callback) callback($(dom));

  $(dom).modal(data.options || {}); 
}

