// $Id$
$(function() {
  $('#field_admin_list select[name="field_list_required"]').change(function() {
    $.ajax({
      type: 'POST',
      dataType: 'html',
      data: 'field_field_id='+$(this).attr('alt')+'&field_change_required=' + $(this).val(),
      success: function(data) {
      }
    });
  });
  $('#field_admin_list select[name="field_list_weight"]').change(function() {
    $.ajax({
      type: 'POST',
      dataType: 'html',
      data: 'field_field_id='+$(this).attr('alt')+'&field_change_weight=' + $(this).val(),
      success: function(data) {
      }
    });
  });
	$(function() {
		var dida_sorttable = '<div class="dida_sorttable_button">';
		dida_sorttable += '<input type="button" class="dida_sorttable_button_cancel" value="取消"/>';
		dida_sorttable += '<input type="button" class="dida_sorttable_button_save" value="保存"/>';
		dida_sorttable += '</div>';
		
		$('#block_system_adminLink').prepend(dida_sorttable);
		$('#block_system_adminLink .dida_sorttable_button').hide();
		
		var result = $("#block_system_adminLink > .block_content").sortable({
			update: function(event, ui) { $('#block_system_adminLink .dida_sorttable_button').show(300); },
			handle: 'h3'
		});
		
		$('#block_system_adminLink .dida_sorttable_button_cancel').click(function() {
			result.sortable('cancel');
			$('#block_system_adminLink .dida_sorttable_button').hide(300);
		});
		
		$('#block_system_adminLink .dida_sorttable_button_save').click(function() {
	    $.ajax({
	      type: 'POST',
	      dataType: 'html',
	      url: Dida.url('ajax'),
	      data: 'module=system&op=admin_menu&' + result.sortable('serialize'),
	      success: function(data) {
	      }
	    });
			$('#block_system_adminLink .dida_sorttable_button').hide(300);
		});
		
		//$("#block_system_adminLink > .block_content").disableSelection();
	});
  
});
