// $Id$
var fields = {ajaxCache: []};
fields.option_count = 0;

fields.ajaxOptions = function($$) {
  var id = $$.attr('alt');
  if (Dida.settings.fields[id]) {
    var opt = Dida.settings.fields[id];
    var d = $$.val();
    $('.field_select_value_'+id).val(d);
    if (fields.ajaxCache[d]) {
      fields.ajaxSuccess(id, $$, fields.ajaxCache[d]);
    } else {
      $.ajax({
        url: opt.url,
        dataType: 'json',
        type: 'POST',
        data: $$.parents('form').serialize() + '&__default_value=' + d,
        success: function(json) {
          fields.ajaxCache[d] = json;
          fields.ajaxSuccess(id, $$, json);
        }
      });
    }
  }
}

fields.ajaxSuccess = function(id, obj, json) {
  obj.nextAll('select').remove();
  if (!json.error) {
    var tmp;
    var label = json.label || Dida.t('field', '不限');
    
    tmp = '<option value="">'+label+'</option>';
    $.each(json.contents, function(i, item) {
    	tmp += '<option value="'+ item.tid +'">'+ item.name +'</option>';
    });
    obj.after('<select id="fields_level_ajax_'+fields.option_count+'" class="field_option_children field_option_children_append" alt="'+id+'" onChange="fields.ajaxOptions($(this));" name="___fields[]">' + tmp + '</select>'); 
    
    if (json.default_value) {
    	$('#fields_level_ajax_'+fields.option_count).val(json.default_value).trigger('change');
    	$('.field_select_value_'+id).val(json.default_value);
    	
    	//fields.ajaxOptions($('#fields_level_ajax_'+fields.option_count));
    }
    
    fields.option_count++;
    //$('.field_select_value_'+id).val("");
  }
}

fields.deleteOption = function($$) {
  var tid = $$.attr('alt');
  var root = $$.closest('.form_item_select');
  root.find('.field_form_selects_option_'+tid).remove();
  $$.parent().remove();
  return false;
}

$(function() {
  $('.field_select_value').each(function() {
    if ($(this).val() == "") {
      $('.field_option_children_'+$(this).attr('alt')).val("").nextAll('select').remove();
    }
  });
  $('.field_option_children').change(function() {
    fields.ajaxOptions($(this));
  });
  $('.form_select_multiple_button').click(function() {
    var root = $(this).closest('.form_item_select');
    var cid = $(this).attr('alt');
    var tid = root.find("input[name='fields[_multiple_"+cid+"]']").eq(0).val();
    if (cid && tid) {
      if (!root.find('.field_form_selects_option_'+tid).eq(0).val()) {
        var name = '';
        root.find('select option:selected').each(function() {
          if ($(this).val()) {
            name += $(this).text() +' › ';
          }
        });
        var html = '<input type="hidden" value="'+tid+'" name="fields['+cid+']['+tid+']" class="field_form_selects_option_'+tid+'" />';
        html += '<span>'+ name + '<a href="javascript:void(0);" alt="'+tid+'" onClick="fields.deleteOption($(this));">' + Dida.t('field', '(移除)') + '</a></span>';
        root.find('.field_form_selects_view').eq(0).append(html);
      }
    }
  });
});
