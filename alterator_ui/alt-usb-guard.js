$(document).ready(function () {

  // workaround to check and uncheck checkbox as usual input
  // state of check  can be changed from lisp 
  // with form-update-value for hidden input 
  $('#checkbox_use_control').change(function () {
    if ($("#checkbox_use_control").attr('checked')) {
      $("#checkbox_use_control_hidden").val("#t");
    }
    else {
      $("#checkbox_use_control_hidden").val("#f");
    }
  });
  $('#checkbox_use_control_hidden').bind('update-value change', function () {
    if ($("#checkbox_use_control_hidden").val() === "#t") {
      $("#checkbox_use_control").attr('checked', true);
    }
    else {
      $("#checkbox_use_control").attr('checked', false);
    }
  });
  $('#checkbox_use_control_hidden').trigger('change');

  /*******************************************************/
  // catch a preset selection event
  // put current value to presets_input_hidden
  $('input[type=radio][name=presets]').change(function () {
    if ($(this).is(':checked')) {
      if ($(this).val()=="radio_white_list" || $(this).val()=="radio_black_list"){
        $("#presets_input_hidden").val("manual_mode");
        $("#hidden_list_type").val($(this).val());
        return;             
      }      
      $("#presets_input_hidden").val($(this).val());
    }
  });

  // set preset selection
  // set radiobutton if presets_input_hidden changes
  $('#presets_input_hidden').bind('update-value change', function () {
    $('input[type=radio][name=presets][value=' + $("#presets_input_hidden").val() + ']').attr('checked', true);
  });
  
  // preselect type-of-list
  $('#hidden_list_type').bind('update-value change', function () {
    if ($('#hidden_list_type').val()==="radio_white_list"){
        $('#preset_manual_white').attr('checked',true);
    }
    else {
        $('#preset_manual_black').attr('checked',true);
    }
  });

  $("#presets_input_hidden").trigger('change');
  $('#hidden_list_type').trigger('change');

  /*******************************************************/
  // delete strings from tables

  localStorage.removeItem("deletedFields");

  $("#delete_rules_from_hash_level").bind('click', "form_list_hash_rules", SaveDeleted);
  $("#delete_rules_from_vid_pid_level").bind('click', "form_list_vidpid_rules", SaveDeleted);
  $("#delete_rules_from_interface_level").bind('click', "form_list_interface_rules", SaveDeleted);
  $("#delete_rules_from_unsorted_level").bind('click', "form_list_unsorted_rules", SaveDeleted);


  /*******************************************************/
  // add strings to tables

  // add new hash-level rule
  var new_hash_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="hash"><span></td>' +
    '<td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_hash", new_hash_row, "#list_hash_rules");

  // add new vidpid-level rule
  var new_vidpid_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="vid"><span></td>' +
    '<td>--</td>' +
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="pid"><span></td>' +
    '<td>--</td><td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_vidpid", new_vidpid_row, "#list_vidpid_rules");

  // add new CC::SS::PP rule
  var new_interface_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="with_interface"><span></td>' +
    '<td>--</td><td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_interfase", new_interface_row, "#list_interface_rules");

  // add new rule to unsorted-rules
  var new_unsorted_rules_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="raw_rule"><span></td>';
  addRuleBehaviorAdd("#add_to_rules_unsorted", new_unsorted_rules_row, "#list_unsorted_rules");
  window.last_append_rule_id = 0;
  
  // on click "save rules"
  // read stored values, put them to hidden input, trigger "ready" event for it
  $("#save_rules_button").bind('click', function () {
    var deletedFields = localStorage.getItem('deletedFields');
    var rules_changes={
      preset_mode: $("#presets_input_hidden").val(),
      deleted_rules:  JSON.parse(deletedFields),
      appended_rules: collectAppendedRules(),
      run_daemon: $("#checkbox_use_control_hidden").val()==="#t" ? "true":"false",
      policy_type: $("#hidden_list_type").val() 
    }
    $("#hidden_manual_changes_data").val(JSON.stringify(rules_changes));
   // $(".manual_appended").remove();
    $("#hidden_manual_changes_data").trigger('rules_json_ready');
  });

  // bind response recieved
  $("#hidden_manual_changes_response").bind('update-value change',function(){
    if ( $(this).val()!==""){  
      var response=JSON.parse( $(this).val());
      $(this).val("");
      if (response["STATUS"] ==="OK"){
        $(".manual_appended").remove();
        localStorage.removeItem("deletedFields");
        $(this).trigger("rules_applied");
        return;
      }
      for (const el of response["rules_BAD"]){
        $('#'+el).css("border", "3px red solid");
      }
      for (const el of response["rules_OK"]){
        $('#'+el).css("border", "2px green solid");
      }  
    }  
 });

 /**
  * file upload
  * Substitute a file with its content, encoded with base64
  * File is substituted because alterator can't handle loading file
  * with unicode-escaped symbols
  */
 $("#load_file_button").bind('click',function(){
  const fileInput = document.getElementById('file_input');
  if (fileInput.files.length > 0) {
    if(fileInput.files[0].size >1024000 ){
        alert("This file is too big. Limit 1 MB");
        fileInput.value="";
    }
    const file = fileInput.files[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = function(event) {
            const fileContent = event.target.result;
            const encodedString = btoa(fileContent);
            let file_encoded = new File([encodedString], "endoded.csv", {
                    type: "text/plain",
                    lastModified: new Date()
                });
            let container = new DataTransfer();
            container.items.add(file_encoded); 
            fileInput.files = container.files;
            // console.log(encodedString);
            $("#load_file_button").trigger("data_ready");
        };
      reader.readAsBinaryString(file);
    }
  } 
 });

}); // .ready

/*******************************************************/

function AddRulesFromFile (data) {
  const rules_json=JSON.parse(data);
  //set policy ratiobuttons
  if (rules_json["STATUS"]==="error"){
    alert(rules_json["ERR_MSG"]);
  }
  if (rules_json.hasOwnProperty('policy')){
    if (rules_json['policy'] === 0){
        $("#form_presets").val("radio_white_list"); 
        $("#hidden_list_type").val("radio_white_list");
    }
    else{
      $("#form_presets").val("radio_black_list"); 
      $("#hidden_list_type").val("radio_black_list");
    }
  }
  // fill the hash table
  if (rules_json.hasOwnProperty("hash_rules") && Array.isArray(rules_json["hash_rules"])){
    rules_json["hash_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      $("#list_hash_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' +
        '<td><span class="alterator-label"><input type="text" class="input_appended" name="hash" value=\''+
        rule["hash"]+
        '\'/><span></td>' +
        '<td>-</td>'+
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });    
  };
  // fill the vidpid rules table
  if (rules_json.hasOwnProperty("vidpid_rules") && Array.isArray(rules_json["vidpid_rules"])){
    rules_json["vidpid_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      $("#list_vidpid_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' +       
        '<td><span class="alterator-label"><input type="text" class="input_appended" name="vid" value=\''+
        rule["vid"]+
        '\'><span></td>' +
        '<td>--</td>' +
        '<td><span class="alterator-label"><input type="text" class="input_appended" name="pid" value=\''+
        rule["pid"]+
        '\'><span></td>' +
        '<td>--</td><td>--</td>'+
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });  
  };
  // fill the interface rule target
  if (rules_json.hasOwnProperty("interf_rules") && Array.isArray(rules_json["interf_rules"])){
    rules_json["interf_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      $("#list_interface_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' +       
        '<td><span class="alterator-label">'+
        '<input type="text" class="input_appended" name="with_interface" value=\''+
        rule["interface"]+
        '\' /><span></td>' +
        '<td>--</td><td>--</td>'+
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });    
  };
  // fill the row rules table
  if (rules_json.hasOwnProperty("raw_rules") && Array.isArray(rules_json["raw_rules"])){
    rules_json["raw_rules"].forEach(function(rule){
      ++window.last_append_rule_id;
      alert (rule["raw"]);
      $("#list_unsorted_rules").append(
        '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'">'+
        '<td><input class="select_appended" type="checkbox"></td>' +
        '<td>--</td>' + 
        '<td><span class="alterator-label">'+
        '<input type="text" class="input_appended" name="raw_rule" value=\''+
        rule["raw"]+
        '\'><span></td>' +
        '<td class="appended_rule_target">' +
        (rule["policy"] === 0 ? "allow" : "block") +
        '</td></tr>');
    });
  };  
  bindCheckBox();
  document.getElementById('file_input').value = '';
};


function addRuleBehaviorAdd(button_id, row_html, table_id) {
  $(button_id).bind('click', function () {
    ++window.last_append_rule_id;
    $(table_id).append(
      '<tr class="manual_appended" id="rule_'+window.last_append_rule_id+'"><td><input class="select_appended" type="checkbox"></td>' +
      '<td>--</td>' +
      row_html + '<td class="appended_rule_target">' +
      ($("#hidden_list_type").val() === "radio_white_list" ? "allow" : "block") +
      '</td></tr>');
    bindCheckBox();

  });
};

/*******************************************************/

function bindCheckBox() {
  $('.select_appended').change(function () {
    if ($(this).is(':checked')) {
      $(this).closest('tr').addClass('selected');
    }
    else {
      $(this).closest('tr').removeClass('selected');
    }
  });
};

/*******************************************************/

function collectAppendedRules() {
  var appended_rules=[];
  // for each appended rule
  $(".manual_appended").each(function () {
    // get a type of rule
    var table_id = $(this).closest('table').attr('id');
    var target = $(this).find("td.appended_rule_target")[0];
    var tr_id = $(this).attr('id');
    // put all field values to array
    var inputs =$(this).find("input.input_appended");
    var fields =[];
    inputs.each(function(i,el){
        var name=$(el).attr('name');
        var val=$(el).val();
        fields.push({[name]:val});
    });
    // put rule to the result array (appended_rules)
    appended_rules.push({table_id:table_id,tr_id:tr_id,target:$(target).text(),fields_arr:fields});
  });
  return appended_rules;
}

/*******************************************************/

// save deleted rules to local storage
function SaveDeleted(table_id) {
  var deletedFields = JSON.parse(localStorage.getItem('deletedFields')) || [];
  $('#' + table_id.data + " tr.selected").each(function () {
    var val = $(this).find("td > span[name='name']").text();
    if (val !== "") {
      deletedFields.push(val);
    }
    $(this).remove();
  });
  localStorage.setItem('deletedFields', JSON.stringify(deletedFields));
}