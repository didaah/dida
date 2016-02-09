// $Id$
$(function() {
  var adminMenu = Dida.getck('admin_menu');
  if (adminMenu) {
    var m = adminMenu.split(',');
    $.each(m, function(i, c) {
      if (c) {
        $(".item_admin_menus .admin_menus:eq("+c+")").hide().attr('alt', 'none')
        .prev('h3').children('.item_click_op').addClass('item-list-hide');
      }
    });
  }
  $('.item_admin_menus .item_click_op').click(function() {
    if ($(this).parent().next('.admin_menus').css('display') == 'block') {
      $(this).addClass('item-list-hide')
      .parent().next('.admin_menus').hide(300).attr('alt', 'none');
    } else {
      $(this).removeClass('item-list-hide')
      .parent().next('.admin_menus').show(300).attr('alt', 'block');
    }
    
    var n = '';
    $('.item_admin_menus .admin_menus').each(function(i) {
      if ($(this).attr('alt') == 'none') {
        n += i + ',';
      }
    });
    var date = new Date();
    date.setTime(date.getTime() + 9999999999);
    document.cookie = "admin_menu=" + n + ";path=/;expires=" + date.toGMTString();
  });
  
  $('.files_view_ckeditorbrowser_insert a').click(function() {
    if (window.opener) {
      var params = Dida.parseQuery(Dida.getUrl());
      if (params['CKEditorFuncNum']) {
        window.opener.CKEDITOR.tools.callFunction(params['CKEditorFuncNum'], $(this).attr('href'), '');
        window.close();
      }
    }
    return false;
  });
  
  $('.files_view_custombrowser_insert a').click(function() {
    if (window.parent) {
      window.parent.ck_insert_img($(this).attr('href'), $(this).attr('title'), $(this).attr('alt'), $(this).attr('link'));
      $(this).remove();
    }
    return false;
  });
  
  $('.files_view_markitupbrowser_insert a').click(function() {
    if (window.parent) {
      window.parent.Dida.markitup.tools.filebrowserInsert($(this).attr('href'), $(this).attr('title'), $(this).attr('alt'), $(this).attr('link'));
      $(this).remove();
    }
    return false;
  });
  
  $('#system_admin_menu_list_didaSystemCustom li a').each(function() {
  	$(this).after('<span class="system_admin_block_link_remove" style="display: none">' + Dida.t('system', '移除这个链接') + '</span>');
  });
  
  $('#system_admin_menu_list_didaSystemCustom li').hover(function() {
  	$(this).children('.system_admin_block_link_remove').show();
  }, function() {
  	$(this).children('.system_admin_block_link_remove').hide();
  });
  
  $('.system_admin_block_link_remove').hover(function() {
  	$(this).addClass('system_admin_block_link_remove_active');
  }, function() {
  	$(this).removeClass('system_admin_block_link_remove_active');
  }).click(function() {
  	var $$ = $(this).prev();
  	if ($$.attr('alt')) {
	    $.ajax({
	      type: 'POST',
	      dataType: 'html',
	      url: Dida.url('ajax'),
	      data: 'module=system&op=block_delete_link&id=' + $$.attr('alt'),
	      success: function(data) {
	    		if (data) {
	    			$$.parent().remove();
	    		}
	      }
	    });
  	} else {
  		$$.parent().remove();
  	}
  });
  
	$('#admin_custom_add_submit').click(function() {
		var path = $('#custom_path').val(), title = $('#custom_title').val();
		if (path && title) {
	    $.ajax({
	      type: 'POST',
	      dataType: 'json',
	      url: Dida.url('ajax'),
	      data: 'module=system&op=block_set_link&path=' + path + '&title=' + title,
	      success: function(data) {
	    		if (data.path) {
	    			$('#custom_path, #custom_title').val("");
	    			var html = '<li><a title="'+data.title+'" href="'+data.path+'">'+data.title+'</a></li>';
	    			$('#system_admin_menu_list_didaSystemCustom li.last').before(html);
	    		}
	      }
	    });
		} else {
			alert(Dida.t('system', '链接和标题不能为空'));
		}
	});
	
});
