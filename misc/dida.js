// $Id$
Dida.dejson = function(data) {
  if ((data.substring(0, 1) != '{') && (data.substring(0, 1) != '[')) {
    return false;
  }
  return eval('(' + data + ');');
};

Dida.getck = function(name) {
  var arr = document.cookie.match(new RegExp("(^| )"+name+"=([^;]*)(;|$)"));
  if (arr != null) {
    return decodeURIComponent(arr[2]);
  } else {
    return null;
  }
};

Dida.gettime = function() {
  var d = new Date();
  return d.getTime();
};

Dida.getsize = function() {
  var de = document.documentElement;
  var w = window.innerWidth || self.innerWidth || (de&&de.clientWidth) || document.body.clientWidth;
  var h = window.innerHeight || self.innerHeight || (de&&de.clientHeight) || document.body.clientHeight;
  arrayPageSize = [w,h];
  return arrayPageSize;
};

Dida.external = function(path) {
  
  if (path.substr(0, 1) == '/') {
    return true;
  }
  
  if (path.indexOf(':') != -1) {
    
    var par = path.split(':');
    var types = {
          'http': 1, 'https': 1, 'ftp': 1, 'news': 1, 'nntp': 1,
          'telnet': 1, 'mailto': 1, 'irc': 1, 'ssh': 1, 'sftp': 1, 'webcal': 1, 'rtsp': 1
        };
    return types[par[0]];
  }
};

Dida.url = function(q, opt) {
  var url = q;
  var external = Dida.external(q);
  
  if (!external) {
    if (Dida.settings.clean_url || url.indexOf('?') != -1) {
      url = Dida.settings.base_path + q;
    } else {
      url = Dida.settings.base_path + '?q=' + q;
    }
  }
  
  if (opt) {
    
    if (url.indexOf('?') == -1) {
      url += '?';
    } else {
      url += '&';
    }
    
    if (typeof opt == 'object') {
      for (var attr in opt) {
        url += attr + '=' +opt[attr] + '&';
      }
      url = url.substr(0, url.length - 1);
    } else {
      url += opt;
    }
  }
  return url;
};

Dida.favorite = function(title, url) {
  if ($.browser.msie) {
    window.external.addFavorite(url, title);
  } else {
    window.sidebar.addPanel(title, url, '');
  }
};

Dida.loca = function(url) {
  if (url == 1) url = '';
  location.href = url ? url : location.href;
};

Dida.onbe = function(msg) {  
  window.onbeforeunload = onbeforeunload_handler;
  function onbeforeunload_handler() {
    var warning = msg ? msg : Dida.t('system', '确认退出？');
    return warning;
  }
};

Dida.onun = function(msg) {  
  window.onunload = onunload_handler;
  function onunload_handler() {
      alert(msg); 
  }
};

Dida.parseQuery = function(query) {
   var Params = {};
   if (!query) {return Params;}
   var Pairs = query.split(/[;&]/);
   
   for ( var i = 0; i < Pairs.length; i++ ) {
      var KeyVal = Pairs[i].split('=');
      if ( ! KeyVal || KeyVal.length != 2 ) {continue;}
      var key = unescape( KeyVal[0] );
      var val = unescape( KeyVal[1] );
      val = val.replace(/\+/g, ' ');
      Params[key] = val;
   }
   return Params;
};

Dida.getUrl = function() {
  var url = location.href;
  return url.replace(/^[^\?]*\??/,'');
};

Dida.messageShow = function(text, opt) {
  var o;
  if (opt) {
    o = {
      timeOut: opt.timeOut || 5000,
      status: opt.status || 'status'
    };
  } else {
    o = {timeOut: 5000, status: 'status'};
  }
  $('body').append('<div class="js_messageShow '+o.status+'">'+text+'</div>');
  window.setTimeout(function() {$('.js_messageShow').remove();}, o.timeOut);
};

Dida.callFunc = function(cb) {
  if (typeof cb == 'function') {
    eval(cb);
  } else {
    var func;
    func = eval(cb);
    return func.apply(null, Array.prototype.slice.call(arguments, 1));
  }
};

Dida.isImage = function(p) {
  var re = /.*(jpg|gif|png|jpeg)$/i;
  return p && re.test(p);
};

Dida.dialog_inline = function(text, opt) {
  $("#dialog_inline_wrapper .dialog_inline_content").html(text);
  var o = {
    width: 450,
    height: 220,
    modal: true,
    inlineId: 'dialog_inline_wrapper',
    title: ''
  };
  $.extend(o, opt);
  $('#dialog_inline_wrapper').dialog(o);
}

Dida.dialog = function(opt) {
  var o = {
    width: 700,
    height: 450,
    title: '', 
    url: '',
    closeText: Dida.t('system', '关闭'),
    autoOpen: true,
    modal: false,
    //bgiframe: true,
    reload: false,
    closeCall: false,
    iframe: false
  };
  
  $.extend(o, opt);
  
  if ($.isPlainObject(Dida.settings.dialogOptions)) {
    $.extend(o, Dida.settings.dialogOptions);
  }
  
  if ($('#dialog_wrapper').size()) {
    $('#dialog_wrapper').dialog('destroy').remove();
  }
  
  $('body').append('<div id="dialog_wrapper" style="display: none;"></div>');
  
  if (isNaN(o.url)) {
    
    var queryString = o.url.replace(/^[^\?]*\??/,'');
    var params = Dida.parseQuery(queryString);
    if (params) {
      o.params = params;
      if (params['width']) {
        o.width = parseInt(params['width']);
      }
      if (params['height']) {
        o.height =  parseInt(params['height']);
      }
      if (params['inlineId']) {
        o.inlineId = params['inlineId'];
      }
      if (params['iframe']) {
        o.iframe = true;
      }
      if (params['closeCall']) {
        o.closeCall = params['closeCall'];
      }
      if (params['reload']) {
        o.reload = params['reload'];
      }
      if (params['modal']) {
        o.modal = params['modal'];
      }
    }
    
  }
  
  if (!$.isFunction(o.close)) {
    o.close = function(event, ui) {
      if (o.closeCall) {
        Dida.callFunc(o.closeCall, o, event, ui);
      }
      if (o.reload) {
        location.reload();
      }
    };
  }
  
  if (o.inlineId) {
    $('#'+o.inlineId).dialog(o);
  } else if (o.iframe) {
    if (!$.isFunction(o.open)) {
      o.open = function() {
        var h = '<div id="dialog_wrapper_loading">';
        h += '<img align="absmiddle" src="'+Dida.settings.base_path+'misc/images/loading.gif" />' + Dida.t('system', '加载中，请稍候…') + '</div>';
        h += '<iframe id="dialog_iframe_wrapper" frameborder="no" border="0" src="'+o.url+'" width="100%"';
        h += ' height="100%" style="display:none"></iframe>';
        
        $(this).append(h);
        
        $('#dialog_iframe_wrapper').load(function() {
          $('#dialog_wrapper_loading').hide();
          $('#dialog_iframe_wrapper').show();
        });
      };
    }
    $('#dialog_wrapper').dialog(o);
    
  } else if (Dida.isImage(o.url)) {
    if (!$.isFunction(o.open)) {
      o.open = function() {
        var _s = Dida.getsize();
        var h = '<div id="dialog_wrapper_loading">';
        h += '<img align="absmiddle" src="'+Dida.settings.base_path+'misc/images/loading.gif" />' + Dida.t('system', '加载中，请稍候…') + '</div>';
        h += '<img id="dialog_image_wrapper" src="'+o.url+'" />';
        $(this).append(h);
        $('#dialog_image_wrapper').load(function() {
          var _sw = $(this).width()+160 < _s[0] ? $(this).width()+80 : _s[0]-80;
          var _sh = $(this).height()+160 < _s[1] ? $(this).height()+80 : _s[1]-80;
          $('#dialog_wrapper').dialog( "option", {
            'width': _sw,
            'height': _sh,
            'position': 'center'
            });
          
          $('#dialog_wrapper_loading').hide();
        });
      };
    }
    
    $('#dialog_wrapper').dialog(o);
    
  } else {
    if (!$.isFunction(o.open)) {
      o.open = function() {
        var h = '<div id="dialog_wrapper_loading">';
        h += '<img align="absmiddle" src="'+Dida.settings.base_path+'misc/images/loading.gif" />' + Dida.t('system', '加载中，请稍候…') + '</div>';
        h += '<div id="dialog_ajax_wrapper" style="display:none"></div>';
        
        $(this).append(h);
        $.post(o.url, o.ajax_vars, function(html) {
          $('#dialog_wrapper_loading').hide();
          $('#dialog_ajax_wrapper').html(html).show();
        });
      };
    }
    
    $('#dialog_wrapper').dialog(o);
    
  }
  
  return false;
};

/**
 * 关闭 dialog，可选择是否刷新页面
 */
Dida.dialog_close = function(reload) {
  $('#dialog_wrapper').dialog('destroy');
  $('#dialog_wrapper').remove();
  if (reload) location.reload();
};

Dida.php = {
  in_array: function(str, arr) {
    for (i = 0; i < arr.length; i++) {
      var thisEntry = arr[i].toString();
      if (thisEntry == str) {
        return true;
      }
    }
    return false;
  }
};

Dida.ajaxSuccess = function(obj, data, type) {
  
  obj.removeClass('ja_loading');
  
  if (obj.attr('type') != 'js') {
    var level = obj.attr('level');
    
    if (level && data == 1) {
      data = level;
    }
    
    var fun = obj.attr('fun');
    
    if (fun) {
      // 调用函数，依次传递：返回值、当前元素、类型
      Dida.callFunc(fun, data, obj, type);
    } else {
    
      switch (data) {
        case 'parent':
          // 删除父级
          obj.parent().remove();
        break;
        case 'two':
          // 删除祖级
          obj.parent().parent().remove();
        break;

        case 'reload':
          // 重载页面
          Dida.loca(); 
        break;

        case 'own':
          // 删除本身
          obj.remove();
        break;
        case 'tr':
          // 删除上级中第一个匹配的 tr
          obj.closest('tr').remove();
        break;
        case 'replace':
          // 替换
          var text = obj.attr('replace') ? obj.attr('replace') : Dida.t('system', '成功');
          if (text) {
            obj.after('<span class="red msgjs">' + text + '</span>');
            obj.remove();
          }
        break;
        default:
          if (data == 1) {
            var text = obj.attr('replace') ? obj.attr('replace') : Dida.t('system', '成功');
            if (text) {
              obj.after('<span class="red msgjs">' + text + '</span>');
              obj.remove();
            }
          } else if (type == 'a') {
            alert(data ? data : Dida.t('system', '操作失败'));
          } else {
            obj.after('<span class="red msgjs">' + (data ? data : Dida.t('system', '操作失败')) + '</span>');
          }
      }
    }
  } else {
    
    eval(data);
    
  }
};

/**
 * js 中的字符串多语言
 * @param module
 *  模块名称
 * @param str
 *  待翻译的字符串
 * @param args
 *  占位符替换值
 * @return string
 */
Dida.t = function(module, str, args) {

  // 读取翻译
  if (Dida.locale && Dida.locale[str]) {
    if (!Dida.locale['__' + module] || !Dida.locale['__' + module][str]) {
      str = Dida.locale[str];
    } else {
      str = Dida.locale['__' + module][str];
    }
  }

  if (args) {
    for (var key in args) {
      switch (key.charAt(0)) {
        case '@':
          args[key] = Dida.checkPlain(args[key]);
        break
        case '!':
          break;
        case '%':
        default:
          args[key] = '<em>' + args[key] + '</em>';
          break;
      }
      str = str.replace(key, args[key]);
    }
  }
  return str;
};

/**
 * 处理字符串的 html 标签
 */
Dida.checkPlain = function(str) {
  str = String(str);
  var replace = { '&': '&amp;', '"': '&quot;', '<': '&lt;', '>': '&gt;' };
  for (var character in replace) {
    var regex = new RegExp(character, 'g');
    str = str.replace(regex, replace[character]);
  }
  return str;
};

$(function() {
  
  $('.button_goto').click(function() {
    location.href = $(this).attr('href');
  });
  
  $('.homepage_button').click(function() {
    if ($.browser.msie) {
      this.style.behavior = 'url(#default#homepage)';
      this.setHomePage($(this).attr('href'));
    } else {
      alert(Dida.t('system', '你的浏览器安全设置过高，不支持此操作。'));
    }
    return false;
  });
  
  $('.favorites_button').click(function() {
    if (!$.browser.mozilla) {
      Dida.favorite($(this).attr('title'), $(this).attr('href'));
      return false;
    }
  });
 
  $('body').append('<div id="dialog_inline_wrapper"><div class="dialog_inline_content"></div></div>');

  $('.dialog_inline').each(function() {
    $('#' + $(this).attr('inlineId')).addClass('dialog_inline_content').hide();
  });

  $('.dialog_inline').click(function() {
    var o = {
      width: $(this).attr('inlineWidth') || 450,
      height: $(this).attr('inlineHeight') || 220,
      modal: true,
      inlineId: $(this).attr('inlineId'),
      title: $(this).attr('inlineTitle') || $(this).attr('title')
    };
    $('#'+o.inlineId).dialog(o);
    return false;
  });

  $('.dida_close_page').live('click', function() {
    if ($.browser.msie) {
      window.opener = null;
      window.close(); 
    } else if ($.browser.mozilla) {
      window.location.href = 'about:blank ';
    } else {
      window.opener = null;
      window.open('', '_self', '');
      window.close(); 
    }
    return false;
  });

  $('.confirm').live('click', function() {
    msg = $(this).attr('alt');
    if (!confirm((msg ? msg : Dida.t('system', '确认此操作吗？')))) {
      return false;
    } else if ($(this).attr('type') == 'button') {
      location.href = $(this).attr('href');
    }
  });
  
  $('.confirmajax').live('click', function() {
    msg = $(this).attr('alt');
    if (confirm((msg ? msg : Dida.t('system', '确认此操作吗？')))) {
      $(this).addClass('ja_loading');
      var $$ = $(this);
      var url = $$.attr('href');
      $.ajax({
        url: url,
        type: $$.attr('method') || 'GET',
        data: {timestamp: Dida.gettime()},
        success: function(data) {
          Dida.ajaxSuccess($$, data, 'a');
        }
      });
    }
    return false;
  });
 
  $('.clickajax').live('click', function() {
    $(this).addClass('ja_loading');
    var $$ = $(this);
    var url = $$.attr('href');
    if ($$.attr('method') != 'POST') {
      $.get(url, {'timestamp': Dida.gettime()}, function(data) {
        Dida.ajaxSuccess($$, data, 'a');
      });
    } else {
      $.post(url, {'timestamp': Dida.gettime()}, function(data) {
        Dida.ajaxSuccess($$, data, 'a');
      });
    }
    return false;
  });

  $('.dida_search_form_field_keyword, .dida_form_field_placeholder_keyword').each(function(){
    if (!$(this).val()) {
      if ($.browser.msie) {
        $(this).val(($(this).attr('title') || $(this).attr('alt'))).addClass('dida_search_form_field_keyword_default');
      } else {
        $(this).attr('placeholder', ($(this).attr('title') || $(this).attr('alt'))).addClass('dida_search_form_field_keyword_default');
      }
    }
  });
  
  $('.dida_search_form_field_keyword, .dida_form_field_placeholder_keyword').focusout(function(){
    if (!$(this).val() && $.browser.msie) {
      $(this).val(($(this).attr('title') || $(this).attr('alt'))).addClass('dida_search_form_field_keyword_default');
    }
    var t = $(this).attr('title') || $(this).attr('alt');
    if (!$(this).val() || $(this).val() == t) {
      $(this).removeClass('dida_search_form_field_keyword_focus');
    }
  }).focusin(function(){
    $(this).addClass('dida_search_form_field_keyword_focus');
    var t = $(this).attr('title') || $(this).attr('alt');
    if (t && $(this).val() == t) {
      $(this).val("").removeClass('dida_search_form_field_keyword_default');
    }
  }).parents('form').submit(function() {
    $(this).find('.dida_search_form_field_keyword,.dida_form_field_placeholder_keyword').each(function() {
      var t = $(this).attr('title') || $(this).attr('alt');
      if ($(this).val() == t) {
        $(this).val("");
      }
    });
  });
  
  $('.dd_form_ajax_field').each(function() {
    if ($(this).hasClass('changeconfirm')) {
      $(this).attr('changeDefault', $(this).val());
    }
  });

  $('.dd_form_ajax_field').change(function() {
    var href = $(this).attr('href');
    if ($(this).hasClass('changeconfirm') && !confirm(Dida.t('system', '确认此操作吗？'))) {
      $(this).val($(this).attr('changeDefault'));
      return false;
    }
    
    if (href) {
      var $$ = $(this);
      $.ajax({
        type: 'POST',
        url: href,
        dataType: 'html',
        data: 'id=' + $(this).attr('alt') + '&value=' + $(this).val(),
        success: function(data) {
          if (data == -1) {
            alert(Dida.t('system', '操作失败'));
          } else if (data == 'two') {
            $$.parent().parent().remove();
          }
        }
      });
    }
  });
  
  $('.form_field_all_check').click(function() {
    dom = $(this).attr('alt');
    $('.' + dom).attr('checked', this.checked ? true : false);
  });
  
  $('a.all_menu_ext').click(function() {
    var dom = $(this).attr('alt');
    $('.' + dom).toggle();
    return false;
  });
  
  $('.dd_form_ajax_form_button').click(function() {
    var getUrl = $(this).attr('href');
    var $$ = $(this);
    if (getUrl) {
      $$.addClass('ja_loading');
      $.ajax({
        type: 'POST',
        url: getUrl,
        dataType: 'script',
        data: $$.parents('form').serialize(),
        success: function(data) { Dida.ajaxSuccess($$, data); },
        error: function(e) { alert('error'); }
      });
    };
    return false;
  });
  
  $('.fieldset-hide').children('.fieldset-wrapper').hide();
  $('fieldset.collapsible > legend.collapse-processed').click(function() {
    $(this).toggleClass('asc').toggleClass('desc').parent().toggleClass('fieldset-hide');
    $(this).next('.fieldset-wrapper').slideToggle(100);
    return false;
  });
  
  $('.dialog').live('click', function() {
    var o = {};
    o.url = $(this).attr('href');
    o.title = $(this).attr('title') || $(this).text();
    
    Dida.dialog(o);
    
    return false;
  });
  
  $('input[class="form_field_all_button"]').click(function() {
    if (confirm(Dida.t('system', '确认此操作吗？'))) {
      var href = $(this).attr('alt');
      var c = $(this).attr('rel');
      $('.' + c).each(function() {
        if (this.checked) {
          var $$ = $(this);
          $$.after('<span class="ja_loading"></span>');
          url = $$.attr('href') ? $$.attr('href') : href;
          var opt = {};
          opt.timestamp = Dida.gettime();
          if (!$$.attr('href')) {
            opt.id = $(this).val();
            opt.name = $(this).attr('alt');
          }
          $.get(url, opt, function(data) {
            $$.next('ja_loading').remove();
            Dida.ajaxSuccess($$, data, 'input');
          });
        }
      });
    }
    return false;
  });
  
  $('.login_msg').click(function() {
    if (!Dida.settings.user_is_login) {
      if (confirm(Dida.t('system', '你需要登录才能进行此操作，立即登录？'))) {
        var u = '';
        if ($(this).attr('redirect')) {
          u = $(this).attr('redirect');
        } else if ($(this).attr('href')) {
          u = $(this).attr('href');
        } else {
          u = location.pathname;
        }
        location.href = Dida.url('user/login', {redirect: u});
      }
      return false;
    }
  });
  
  $('.confirm_msg').live('click', function() {
    alert($(this).attr('title'));
    return false;
  });
 
  
  if (Dida.settings.multi) {
    for (var attr in Dida.settings.multi) {
      var element = Dida.settings.multi[attr];
      $(element).each(function() {
        $('#multi_' + this.dom).MultiFile({'list' : '#multi_list_' + this.dom});
      });
    }
  }
  if (Dida.settings.edit) {
    $(Dida.settings.edit).each(function() {
      $(this.dom).editable(null, this.opt);
    });
  }
  
  $('.pager_form_go_input').blur(function() {
    var id = $(this).attr('alt');
    var s = Dida.settings.pager[id];
    var t = parseInt($(this).val());
    var go = 0;
    if (t) {
      t = t - 1;
      if (t != s.current && t < s.sum) {
        go =  t > s.sum ? s.sum : t;
        var h = location.href;
        
        if (go > 0) {
          if (h.indexOf('page=') != -1) {
            var re = /page=\d*/i;
            h = h.replace(re, 'page=' + go);
          } else {
            h = Dida.url(h, {'page': go});
          }
        } else {
          var re = /[\?|&]page=\d*/i;
          h = h.replace(re, '');
        }
        location.href = h;
      }
    }
  });
  
  $('.dida_tips').each(function() {
    var dom = $(this).attr('alt');
    $(dom).hide().addClass('dida_tips_view');
  });
  
  $('.dida_tips').live('mouseover', function(e){
    var dom = $(this).attr('alt');
    var a = Dida.getsize();
    var s = {top: e.pageY+10};
    if (e.pageX > 800) {
      s.right =  a[0]-e.pageX;
    } else {
      s.left = e.pageX+55;
    }
    $(dom).css(s).show();
  });
  
  $('.dida_tips').live('mouseout', function(){
    var dom = $(this).attr('alt');
    $(dom).hide();
  });
  
  if (Dida.settings.farbtastic) {
    for (var attr in Dida.settings.farbtastic) {
      var element = Dida.settings.farbtastic[attr];
      $(element).each(function() {
        var f = $.farbtastic(this.dom);
        var p = $(this.dom).css('opacity', 0.25);
        var selected;
        $(this.items)
          .each(function () { f.linkTo(this); $(this).css('opacity', 0.35); })
          .focus(function() {
            if (selected) {
              $(selected).css('opacity', 0.35).removeClass('colorwell-selected');
            }
            f.linkTo(this);
            p.css('opacity', 1);
            $(selected = this).css('opacity', 1).addClass('colorwell-selected');
          });
      });
    }
  };
 
  $('.table_fixed_header').each(function(i) { 
    var w = $('#content').width()-20;
    var width = w;
    if ($(this).attr('min-width')) {
      width = parseInt($(this).attr('min-width'));
    };

    if (width < w) width = w;

    var id = 'table_fixed_header_wrapper_index_' + i;
    $(this).css('width', width+'px');
    $(this).wrap('<div class="table_fixed_header_wrapper" id="'+id+'"></div>');
    var table = '#'+id+'_table';
    $(this).before('<table id="'+id+'_table" class="table_fixed_header_wrapper_clone"></table>');
    $('thead', this).clone().prependTo(table);
    var css = $(this).offset();
    css.position = 'fixed';
    css.width = width + 'px';
    css['text-align'] = $(this).css('text-align');

    $('thead tr:first th', this).each(function(i) {
      $('thead tr:first th', table).eq(i).css('width', $(this).width()+'px');
    });

    $(table).append('<tbody></tbody>');
    $('tr.table_fixed_rows', this).clone().prependTo(table + ' tbody');
    $('tr.table_fixed_rows td', this).each(function(i) {
      $('tr.table_fixed_rows td', table).eq(i).css('width', $(this).width()+'px');
    });
    $(table).css(css).hide();
  });
 
  $('.table_fixed_header_wrapper').scroll(function(i) {
    var t = $(this).scrollLeft();
    var s = $(this).offset();
    $('.table_fixed_header_wrapper_clone', this).css('left', (s.left-t)+'px');
  });

  $(window).scroll(function() {
    var t = $(this).scrollTop();
    $('.table_fixed_header').each(function(i) { 
      var id = '#table_fixed_header_wrapper_index_' + i + '_table';
      if (t > $(this).offset().top) {
        $(id).css({top: 50}).show();
      } else {
        $(id).hide();
      }
    });
  });

  //自动完成
  if (Dida.settings.auto) {
    var ui_auto = {};
    $(Dida.settings.auto).each(function(i, item) {
      if (item.dom) {
        ui_auto[i] = item;
        if (ui_auto[i].url) {
          ui_auto[i].cache = {};
          ui_auto[i].source = function(request, response) {
            if (ui_auto[i].cache[request.term] != undefined) {
              response(ui_auto[i].cache[request.term]);
              return;
            }
            
            $.ajax({
              url: ui_auto[i].url,
              dataType: 'json',
              type: 'POST',
              data: {value: request.term},
              success: function(data) {
                if (!data.error && data.contents) {
                  ui_auto[i].cache[request.term] = data.contents;
                  response(data.contents);
                } else {
                  if (ui_auto[i].range && ($.browser.mozilla || $.browser.msie)) {
                    $(ui_auto[i].dom).val("");
                  }
                  ui_auto[i].cache[request.term] = [];
                  response([]);
                }
              }
            });
          };
        }
        $(ui_auto[i].dom).autocomplete(ui_auto[i]);
        if ($.browser.mozilla) {
          /**
           * firefox 中文输入法 bug
           */
          $(ui_auto[i].dom).bind("text", function() {
            $(this).autocomplete('search');
          });
        }
      }
    });
  }
  
  if (Dida.settings.sort) {
    var element = Dida.settings.sort;
    $(element).each(function(i) {
      var $$ = this;
      $(this.wid).after('<span class="ui-icon ui-icon-arrowthick-2-n-s"></span>');
      $(this.dom).after('<div class="messages sort_messages sort_messages_'+ i +'" style="display: none"></div>');
      $(this.dom).sortable({
         change: function(event, ui) {$('.sort_messages').show().text(Dida.t('system', '提示：排序已变动，请提交保存')); $(this.dom).sortable("serialize"); }
      });
    });
  };
  
  if (Dida.settings.markItUp && typeof(Dida.markitup) == 'object') {
    $(Dida.settings.markItUp).each(function() {
      if (this.dom) {
        var $$ = this;
        var type = $$.type || 'html';
        $(this.dom).markItUp(Dida.markitup[type]($$.options));
      }
    });
  }
  
  // ajax 验证
  if (Dida.settings.ajax_validate) {
 	
    $.extend($.validator.messages, {
      required: Dida.t('system', '不能为空'),
      remote: Dida.t('system', '不正确的输入，请修改'),
      email: Dida.t('system', '请输入合法的电子邮件地址'),
      url: Dida.t('system', '请输入有效的URL'),
      date: Dida.t('system', '请输入有效的日期'),
      dateISO: Dida.t('system', '请输入(ISO)标准日期格式，如：2009-08-28'),
      number: Dida.t('system', '只允许输入数字'),
      digits: Dida.t('system', '只允许输入整数'),
      creditcard: Dida.t('system', '请输入有效的信用卡号码'),
      equalTo: Dida.t('system', '两次输入不一致'),
      accept: Dida.t('system', '请输入有效的扩展名'),
      maxlength: $.validator.format(Dida.t('system', '最多允许 {0} 个字符')),
      minlength: $.validator.format(Dida.t('system', '最少需要 {0} 个字符')),
      rangelength: $.validator.format(Dida.t('system', '长度必须在 {0} 到 {1} 之间')),
      range: $.validator.format(Dida.t('system', '仅允许 {0} 到 {1} 之间的值得')),
      max: $.validator.format(Dida.t('system', '必须小于或等于 {0}')),
      min: $.validator.format(Dida.t('system', '必须大于或等于 {0}'))
    });

    $.validator.addMethod("required", function(value, element, param) {
      if ( !this.depend(param, element) )
        return "dependency-mismatch";
      switch( element.nodeName.toLowerCase() ) {
        case 'select':
          var val = $(element).val();
          return val && val.length > 0;
        case 'input':
          if ( this.checkable(element) ) {
            if (element.type == 'checkbox') {
              var name = $(element).parent('.form_checkbox_option').attr('alt');
              if (name) {
                cd = false;
                var errorClass = this.settings.errorClass;
                $(element).parents('form').find('.'+name).each(function() {
                  if ($(this).attr('checked')) {
                    cd = true;
                    $(element).parents('form').find('.'+name).each(function() {
                      $(this).next('label.'+errorClass).hide();
                    });
                    return cd;
                    
                  }
                });
                
                return cd;
              }
            }
            
            return this.getLength(value, element) > 0;
          }
        default:
          return $.trim(value).length > 0;
      }
    });
 
    $(Dida.settings.ajax_validate).each(function(i) {
      var o = this;
      if (o.ajax_submit) {
        o.submitHandler = function(form) {
          $(form).ajaxSubmit(o.options);
        };
      }
      $('#'+ o.form_id).validate(o);
    });
       
  };
  
  /**
   * jquery ui 时间控件
   */
  if (Dida.settings.uidate) {
    $.datepicker.setDefaults($.datepicker.regional['zh-CN']);
    var element = Dida.settings.uidate;
    $(element).each(function(i) {
      var o = this;
      o.showStatus = true;
      o.showOn = "both";
      o.buttonImage = Dida.settings.base_path + "misc/images/calendar.gif";
      o.buttonImageOnly = true;
      
      if (o.start || o.end) {
        o.onClose = function(text, instance) { 
          uidate_vali(text, instance, (instance.settings.start == 'ok' ? 'start' : 'end'));
        };
      }

      o.beforeShow = function() {
        setTimeout(function () {$('#ui-datepicker-div').css("z-index", 9999);}, 100); 
      }
      
      if (o.showTime) {
        o.duration = '';
        o.showTime = true;
        if (typeof(o.showSecond) == 'undefined') {
          o.showSecond = true;
        }
        if (o.showSecond && typeof(o.timeFormat) == 'undefined') {
          o.timeFormat = 'hh:mm:ss';
        }
        if (o.dateDisabled) {
          $(o.dom).timepicker(o).focus(function() { this.blur(); });
        } else {
          $(o.dom).datetimepicker(o).focus(function() { this.blur(); });
        }
      } else {
        $(o.dom).datepicker(o).focus(function() { this.blur(); });
      }
      
    });
  };
 
  function uidate_vali(text, instance, type) {
    var val_end, val_start;

    if (type == 'start') {
      val_start = text;
      val_end = $(instance.settings.end).val();
    } else {
      val_end = text;
      val_start = $(instance.settings.start).val();
    }

    if (val_end && val_start) {
      re = /[^0-9]/g;
      val_end = val_end.replace(re, '');
      val_start = val_start.replace(re, '');
      if (val_end <= val_start) {
        $(instance.settings.dom).eq(0).val('');
        alert(Dida.t('system', '结束日期必须大于开始日期'));
      }
    }
    return false;
  };
});
