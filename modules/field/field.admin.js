// $Id$

$(function() {
	var _k = $('#field_form_type_data_validate_type').val();
	if (_k) {
		$('fieldset.field_type_textfield_settings_'+_k).show(200);
	};
	$('#field_form_type_data_validate_type').change(function() {
		var v = $(this).val();
		$('fieldset.field_type_textfield_settings').hide();
		if (v) {
			$('fieldset.field_type_textfield_settings_'+v).show(200);
		}
	});
});