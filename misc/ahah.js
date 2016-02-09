// $Id$
$(function() {
  for (var attr in settings.ahah) {
    var element = settings.ahah[attr];
    $(element).each(function() {
      ahah = this;
      $('#' + attr).bind(ahah.event, function() {
        if (ahah.callFunction) {
          Dida.callFunc(ahah.callFunction, ahah);
        } else {
          var d = '&__default_value=' + $('#' + attr).val();
          $.ajax({
            url: ahah.url,
            data: $('#' + attr).parents('form').serialize() + d,
            success: function(r) {
              if (ahah.callFunction) {
                Dida.callFunc(ahah.callFunction, r, $('#' + attr));
              }
            },
            dataType: ahah.dataType || 'html',
            type: ahah.type || 'POST'
          });
        }
      });
    });
  }
});