// $Id$
jQuery.fn.extend({
  menuLevel: function(o) {
  
    $(this).addClass('js_menu_level_root').find('ul').css({position: 'absolute', display:'none', visibility:'visible'}).each(function() {
      if ($(this).parent('li').parent('ul').hasClass('js_menu_level_root')) {
        $(this).parent('li').addClass('js_menu_level_parent');
      } else {
        $(this).parent('li').addClass('js_menu_level_children');
      }
    });
    
    $(this).children('li').addClass('js_menu_level_one_li').children('a').each(function() {
      $(this).addClass('js_menu_level_one_a').wrapInner(function() {
        return '<span/>';
      });

      if ($(this).hasClass('active')) {
        $(this).addClass('js_menu_level_one_a_active');
      }
    });
    
    $('.js_menu_level_parent').css({position: 'relative'}).hover(function() {
      var _c = $(this).children('ul').eq(0);
      _c.css({left: (this.offsetWidth - _c.width())/2 + 'px', top: this.offsetHeight, display:'block'});
      $(this).addClass('active');
    },function() {
      $(this).removeClass('active').children('ul').eq(0).hide();
    });
    
    $('.site_menu_children .active').each(function() {
      $(this).parents('li').addClass('js_active');
    });

    $('.js_menu_level_children').css({position: 'relative'}).hover(function() {
      var _c = $(this).children('ul').eq(0);
      var _s = {left:this.offsetWidth-2, top: -1, display: 'block'};
      
      if ($(window).width() - $(this).offset().left -  this.offsetWidth - _c.width() < 20) {
        _s.left = -(this.offsetWidth > _c.width() ? this.offsetWidth : _c.width());
      }
      _c.css(_s);
    },function() {
      $(this).children('ul').eq(0).hide();
    }); 
  },
 
  didaTabs: function(o){
    var opt = {
      event: 'click',
      className: '.dida_tabs',
      varyTime: 5000,
      mouseout: null,
      bodyDom: 'href',
      auto: false
    };
    
    $.extend(opt, o);
    
    var autoDom, domCount, currentId, showTimer;;
    
    var root = this;
    $$ = this.find(opt.className);
    
    function show(obj){
      if(typeof obj != 'object'){
        if(currentId == domCount) currentId = 0;
        obj = $$.eq(currentId);
      }
      
      if(typeof obj == 'object'){
       var dom = obj.attr(opt.bodyDom);
        
        currentId = obj.index();
        currentId += 1;
        root.find(opt.className).each(function(){
          $(this).removeClass('active');
          var dom = $(this).attr(opt.bodyDom);
          $(dom).hide();
        });
        
        $(dom).show();
        obj.addClass('active');
      }
    }
    
    function hide(obj){
      if(typeof obj == 'object'){
        var dom = obj.attr(opt.bodyDom);
        $(dom).hide();
        $(dom).mouseover(function(){$(this).show();});
      }
    }
    
    $$.each(function(i){
      $(this).css({cursor: 'pointer'});
      var dom = $(this).attr(opt.bodyDom);
 
      if(opt.auto){
        $(dom).hover(function(){
          clearInterval(showTimer);
        },function(){
          showTimer = window.setInterval(show, opt.varyTime);
        });
      }
      
      if($(dom).attr('alt') != 'default'){
        $(dom).hide();
      }
      
    });
    
    if(opt.auto){
      currentId = 0;
      domCount = $$.size();
      showTimer = window.setInterval(show, opt.varyTime);
      $$.hover(function(){
        clearInterval(showTimer);
      },function(){
        showTimer = window.setInterval(show, opt.varyTime);
      });
    }
    
    if(opt.mouseout == 'hide'){
      $(this).find(opt.className).each(function(){
        $($(this).attr(opt.bodyDom)).hover(function(){ 
          $(this).show().attr('alt', 'block');
        }, function(){
          $(this).hide().attr('alt', 'none');
        });
      });
      root.mouseout(function(){
        $(this).find(opt.className).each(function(){
          if($($(this).attr(opt.bodyDom)).attr('alt') == 'none'){
            $(this).removeClass('active');
            $($(this).attr(opt.bodyDom)).hide();
          }
        });
      });
    }
    
    switch(opt.event){
      case 'click':
        $$.click(function(){ 
          show($(this));
          return false;
       });
      break;
      default:
        $$.hover(function(){
          show($(this));
          return false;
        },function(){
          return false;
        });
    }
  },

  didaShow: function(o) {
    var opt = {
      fadeOutTime: 500,
      fadeInTime: 500,
      dom: '',
      direction: 'y', //
      isTab: true,
      varyTime: 4000
    }

    $.extend(opt, o);
    
    var _data, _self, _dataBtn;

    opt.veryTime += opt.fadeOutTime+opt.fadeInTime;

    if (opt.dom) {
      _data = $(opt.dom + ' .focus_change_list li');
      _target = $(opt.dom + ' .focus_change_list');
      _self = $(opt.dom);
    } else if (this.attr('id')) {
      _data = $('#' + this.attr('id') + ' .focus_change_list li');
      _target = $('#' + this.attr('id') + ' .focus_change_list');
      _self = $('#' + this.attr('id'));
    } else {
      _data = $('.focus_change_list li', this);
      _target = $('.focus_change_list', this);
      _self = $(this);
    }

    _self.css({'overflow': 'hidden', 'position': 'relative'});

    if (opt.direction == 'y') {
      _target.css({'position': 'absolute', 'left': 0, 'top': 0, 'width': '100%'});
    } else {
      _target.css({'position': 'absolute', 'left': 0, 'top': 0, 'width': _self.width()*_data.size() + 'px'});
    }

    var currentId = 0, timeInt, imgCount, showTimer;
    
    imgCount = _data.size();

    if (imgCount > 0) {
      if (opt.isTab) {
        var html = '<div class="focus_change_btn">';
        for (var i = 1, j = 0; i <= imgCount; i++, j++) {
          html += '<a href="#" class="focus_change_btn_item_' + i + '">';
          if (_data.eq(j).attr('thumb')) {
            html += '<img src="'+ _data.eq(j).attr('thumb') +'" />';
          } else {
            html += j;
          }
          html += '</a>';
        }
        html += '</div>';
        _self.append(html);
        _dataBtn = _self.find('.focus_change_btn a');
        _dataBtn.eq(0).addClass('current');
      }

      var is_show = 1, _height = 0, _width = 0;

      function show() {
        if (!is_show) return;

        is_show = 0;

        if (opt.isTab) _dataBtn.eq(currentId).removeClass('current');

        if (opt.direction == 'y') {
          _height = _data.eq(currentId).height();
          for (var i = 0; i < currentId; i++) {
            _height += _data.eq(i).height();
          }
          currentId += 1;
          if (currentId >= imgCount) {
            currentId = 0;
            _height = 0;
          }
          _target.animate({top: '-' + _height + 'px' }, {queue: false, duration: 500});
        } else {
           _width = (currentId+1)*_self.width();
          currentId += 1;
          if (currentId >= imgCount) {
            currentId = 0;
            _width = 0;
          }
          _target.animate({left: '-' + _width + 'px' }, {queue: false, duration: 500});
        }

        if (opt.isTab) _dataBtn.eq(currentId).addClass('current');
        
        is_show = 1;
      }
      
      showTimer = window.setInterval(show, opt.varyTime);
      
      $(opt.dom).hover(function() {
        clearInterval(showTimer)
      }, function() {
        showTimer = window.setInterval(show, opt.varyTime);
      });
     
      if (opt.isTab) {
        _dataBtn.click(function() {
          return false;
        });

        _dataBtn.hover(function() {
          clearInterval(showTimer)
          _dataBtn.removeClass('current');
          if (!is_show) return;
          var h = $(this).index();
          if (h != currentId) {
            currentId = h > 0 ? h - 1 : imgCount-1;
            show();
          }
        }, function() {
          showTimer = window.setInterval(show, opt.varyTime);
        });
      }
    }
  },

  didaScroll: function(o) {
    var opt = {
      varyTime: o.varyTime || 3000
    };
    var currentId = 0;
    var visibleCount = 0;
    var $$ = this;
    var c = $$.children().size();
    
    switch(o.direction) {
      case 'x':
        var chWidth = parseInt($$.children().eq(0).css('width'));
        var dowWidth = c*chWidth;
        var ac;
        $$.css({width: dowWidth, overflow: 'hidden', position: 'absolute'}).parent().css({position: 'relative'});
        
        if (o.visibleCount > 1) {
          visibleCount = c - o.visibleCount;
        } else {
          visibleCount = c;
        }
        
        function showX() {
          currentId += 1;
          if (currentId == visibleCount) currentId = 0;
          $$.animate({left:-(o.size*currentId)}, 500);
        }
        ac = window.setInterval(showX, opt.varyTime);
        $$.hover(function() {
          clearInterval(ac)
        }, function() {
          ac = window.setInterval(showX, opt.varyTime);
        });
        if (o.nextButton) {
          $(o.nextButton).click(function() {
            showX();
            return false;
          });
        }
        if (o.prevButton) {
          $(o.prevButton).click(function() {
            if (currentId > 1) {
              currentId -= 2;
              showX();
            }
            return false;
          });
        }
      break;
      case 'y':
        
        //var chWidth = parseInt($$.children().eq(0).css('height'));
        var dowWidth = c*o.size;
        var bc;
        $$.css({height: dowWidth, overflow: 'hidden', position: 'absolute'}).parent().css({position: 'relative'});
        
        if (o.visibleCount > 1) {
          visibleCount = c - o.visibleCount;
        } else {
          visibleCount = c;
        }
        
        function showY() {
          currentId += 1;
          if (currentId == visibleCount) currentId = 0;
          $$.animate({top:-(o.size*currentId)}, 700);
        }
        bc = window.setInterval(showY, opt.varyTime);
        $$.hover(function() {
          clearInterval(bc)
        }, function() {
          bc = window.setInterval(showY, opt.varyTime);
        });
    }
  }

});
