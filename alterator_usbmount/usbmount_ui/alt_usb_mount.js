function InitUi(health) {
    // click events
    BindRowSelect();
    BindDoubleClick();
    // button click events
    BinEditRow();
    BindReset();
    BindDeleteCheckedRows();
    BindRowCheckbox();
    document.getElementById('add_rule_btn').addEventListener('click', function (e) {
        AppendRow();
    });
    let delete_curr_btn = document.getElementById('delete_curr_btn');
    DisableButton(delete_curr_btn);
    delete_curr_btn.addEventListener('click', DeleteSelectedRow);
    // keybord "Insert" on a table
    let table_body = document.getElementById('rules_list_table').tBodies[0];
    table_body.addEventListener('keydown', function (e) {
        if (e.key == "Insert") {
            document.getElementById('add_rule_btn').dispatchEvent(new Event('click'));
        }
        if (e.key == "Delete" && table_body.querySelectorAll('tr.tr_selected').length != 0) {
            document.getElementById('delete_curr_btn').dispatchEvent(new Event('click'));
        }
    });

    // enable the "Create a rule button if some row is selected"
    DisableButton(document.getElementById('btn_create_rule'));
    document.getElementById('devices_list_table').tabIndex = 0;
    document.getElementById('devices_list_table').addEventListener('click', function (e) {
        let table = document.getElementById('devices_list_table');
        let selected = table.tBodies[0].querySelectorAll('tr.selected');
        let button = document.getElementById('btn_create_rule');
        if (selected.length > 0) EnableButton(button);
        else {
            DisableButton(button);
        }
    });
    // double click on a device to create a rule
    document.getElementById('devices_list_table').addEventListener('dblclick', function (e) {
        const clicked_row = e.target.closest('tr');
        if (clicked_row.classList.contains('selected')) {
            document.getElementById('btn_create_rule').dispatchEvent(new Event('click'));
        }
    });
    // create a rule
    document.getElementById('btn_create_rule').addEventListener('click', function (e) {
        let table = document.getElementById('devices_list_table');
        let selected = table.tBodies[0].querySelectorAll('tr.selected');
        if (selected.length <= 0) return;
        CreateRuleForDevice(selected[0]);
    });
    // SAVE button
    DisableButton(document.getElementById('btn_save'));
    document.getElementById('btn_save').addEventListener('click',SaveRules);

    document.getElementById('checkbox_daemon_status').addEventListener('change',ShowLableForTooggle);

    // toggle status switch
    SetHealthStatus(health);
    // run the service
    document.getElementById('btn_save_status').addEventListener('click',(e)=>{
        let toggle_daemon=document.getElementById('checkbox_daemon_status');
        if (toggle_daemon.checked){
            e.target.dispatchEvent(new Event("btn_run_service"));
        } else{
            e.target.dispatchEvent(new Event("btn_stop_service"));
        }
    });


    InitLogs();
   
}

function SetHealthStatus(health){
    let toggle_daemon=document.getElementById('checkbox_daemon_status');
    if (health==="OK") toggle_daemon.checked=true;
    else toggle_daemon.checked=false;
    localStorage.setItem("health_status",health);
    toggle_daemon.dispatchEvent(new Event('change')); 
    if (health!=="OK"){
        document.getElementById('id_tip_run_service').classList.remove('hidden');
        document.getElementById('interface_wrap').classList.add('hidden');       
    }else{
        document.getElementById('id_tip_run_service').classList.add('hidden');
        document.getElementById('interface_wrap').classList.remove('hidden');    
    }         
}

function UnEscape(htmlStr) {
    htmlStr = htmlStr.replace(/&#9;/g , "\t");	 
    htmlStr = htmlStr.replace(/&#10;/g , "\n");     
    htmlStr = htmlStr.replace(/&#34;/g , "\"");  
    htmlStr = htmlStr.replace(/&#39;/g , "\'");   
    htmlStr = htmlStr.replace(/&#92;/g , "\\");
    return htmlStr;
  }

function LockLogPagination(){
    DisableButton(document.getElementById('btn_prev_page'));
    DisableButton(document.getElementById('btn_next_page'));
}
  

function InitLogs(){
    document.getElementById('show_logs_button').addEventListener('click',e=>{
        document.getElementById('logs_table').classList.remove('hidden');
        e.target.classList.add('hidden');
        document.getElementById('hide_logs_button').classList.remove('hidden');
    });

    document.getElementById('hide_logs_button').addEventListener('click',e=>{
        document.getElementById('logs_table').classList.add('hidden');
        e.target.classList.add('hidden');
        document.getElementById('show_logs_button').classList.remove('hidden');
    });

    document.getElementById('btn_prev_page').addEventListener('click',e=>{
        if (window.log_current_page<window.log_total_pages){
            ++window.log_current_page;
            document.getElementById('hidd_inp_curr_page').value=window.log_current_page;
            LockLogPagination();
            document.getElementById('hidd_inp_curr_page').dispatchEvent(new Event("page_change"));
        }        
    });

    document.getElementById('btn_next_page').addEventListener('click',e=>{
        if (window.log_current_page>0){
         --window.log_current_page;
         document.getElementById('hidd_inp_curr_page').value=window.log_current_page;
         LockLogPagination();
         document.getElementById('hidd_inp_curr_page').dispatchEvent(new Event("page_change"));
        }
    });

    document.getElementById('log_search_button').addEventListener('click',e=>{
        window.log_current_page=0;
        document.getElementById('hidd_inp_curr_page').value=window.log_current_page;
        let input_val=document.getElementById('log_search_input').value.replace(/[&<>"']/g, "");
        document.getElementById('log_search_input').value=input_val;
        document.getElementById('hidd_inp_filter').value=input_val;
        DisableButton(e.target);
        LockLogPagination();
        document.getElementById('hidd_inp_curr_page').dispatchEvent(new Event("page_change"));
    });

    document.getElementById('log_search_input').addEventListener('keydown', function(event) {
        if (event.key === 'Enter') {
          document.getElementById('log_search_button').dispatchEvent(new Event('click'));
        }
    });
}

function SetLogData(data){
    try{
      let obj_data=JSON.parse(data);
      window.log_current_page=obj_data.current_page;
      window.log_total_pages=obj_data.total_pages;
      document.getElementById('log_textarea').textContent=UnEscape(obj_data.data.join('\n'));
      document.getElementById('hidd_inp_curr_page').value=obj_data.current_page;
      document.getElementById('span_curr_page').textContent=" "+(window.log_current_page+1)+" ";
      document.getElementById('span_total_pages').textContent=" "+(window.log_total_pages+1);
      if ( window.log_current_page===0){
        DisableButton(document.getElementById('btn_next_page'));
      } else if (window.log_current_page>0){
        EnableButton(document.getElementById('btn_next_page'));
      }
      if (window.log_current_page>= window.log_total_pages){
        DisableButton(document.getElementById('btn_prev_page'));
      } else {
        EnableButton(document.getElementById('btn_prev_page'));
      }
      EnableButton(document.getElementById('log_search_button'));
    }
    catch(e){
        console.log(e.message);
    }
}



function ShowLableForTooggle(e){
    let toggle=e.target;
    if (!toggle) return;
    let run_lbl=document.getElementById('lbl_run_daemon');
    let stop_lbl=document.getElementById('lbl_stop_daemon');
    let health= localStorage.getItem('health_status');
    let button=document.getElementById('btn_save_status');
    if (toggle.checked== true){
        run_lbl.classList.remove('hidden');
        stop_lbl.classList.add('hidden');
        health==="OK" ? DisableButton(button): EnableButton(button);              
    }else{
        run_lbl.classList.add('hidden');
        stop_lbl.classList.remove('hidden');
        health==="OK" ? EnableButton(button): DisableButton(button);                  
    }
}

function CreateRuleForDevice(alterator_row) {
    if (!alterator_row) return;
    let alterator_labels = alterator_row.querySelectorAll('span.alterator-label');
    let vid;
    let pid;
    let serial;
    let owned;
    alterator_labels.forEach(lbl => {
        let name = lbl.getAttribute('name');
        if (name === 'lbl_vid') vid = lbl.textContent.trim();
        if (name === 'lbl_pid') pid = lbl.textContent.trim();
        if (name === 'lbl_serial') serial = lbl.textContent.trim();
        if (name === 'lbl_status') owned = lbl.textContent.trim() === "owned";
    });
    if (!vid || !pid || !serial || owned === undefined) return;
    let rules_tbody = document.getElementById('rules_list_table').tBodies[0];
    // owned - edit existing rule
    if (owned) {

        // find the rule in rules 
        let local_data = localStorage.getItem('rules_data');
        let rule_id = -1;
        try {
            let json_arr = JSON.parse(local_data);
            let rule = json_arr.find((element) =>
                element.perm.device.vid === vid &&
                element.perm.device.pid === pid &&
                element.perm.device.serial === serial
            );
            if (rule) rule_id = rule.id;
        }
        catch (e) {
            console.log(e.message);
            return;
        }
        if (rule_id < 0) return;
        // the rule is found, find in rules table
        let rules_table_ids = rules_tbody.querySelectorAll('td.rule_id');
        let row;
        rules_table_ids.forEach(td_id => {
            let span_val = td_id.querySelector('span.span_val');
            if (span_val && span_val.textContent == rule_id) {
                row = td_id.parentElement;
            }
        });
        if (row) {
            ResetRow(row);
            RowSelect(document.getElementById('rules_list_table'),row);
            MakeTheRowEditable(row);
        }
    }
    else {
        //check if already created in rules table
        let existing_row;
        let appended_rules = rules_tbody.querySelectorAll('tr.row_appended_by_user');
        if (appended_rules){
            appended_rules.forEach(td => {
                if (td.querySelector('span.rule_vid_val').textContent.trim() === vid &&
                    td.querySelector('span.rule_pid_val').textContent.trim() === pid &&
                    td.querySelector('span.rule_serial_val').textContent.trim() == serial) {
                    existing_row = td;
                }
            });
        }
        if (existing_row) {
            RowSelect(document.getElementById('rules_list_table'),existing_row);
            MakeTheRowEditable(existing_row);
        }
        else {
            let new_row=AppendRow(CreateRuleTemplate(vid, pid, serial));
            RowSelect(document.getElementById('rules_list_table'),existing_row);
        }
    }
}


// ------------------- rules list -------------------------

// update table with data and store data to local storage
function UpdateRulesList(data) {
    try {
        let json_arr = JSON.parse(data);
        let table = document.getElementById('rules_list_table');
        localStorage.setItem('rules_data', data);
        ClearList(table);
        for (index in json_arr) {
            AppendTheRule(json_arr[index], index);
        }
        BindTableHeaderCheckbox(table);
        BindDoubleClick();
        BindRowSelect();
        BindRowCheckbox();
    } catch (e) {
        console.log(e.message);
    }
}

// reset the whole list
function ResetRulesList() {
    try {
        let data = localStorage.getItem('rules_data');
        let json_arr = JSON.parse(data);
        let table = document.getElementById('rules_list_table');
        ClearList(table);
        for (index in json_arr) {
            AppendTheRule(json_arr[index], index);
        }
        BindTableHeaderCheckbox(table);
        BindDoubleClick();
        BindRowSelect();
        BindRowCheckbox();
        ShowToolTipIfSomeInvalid();
    } catch (e) {
        console.log(e.message);
    }
}

// clear list;
function ClearList(table) {
    let rows = table.getElementsByTagName('tr');
    while (rows.length > 1) {
        table.deleteRow(1);
    }
}

// append rule from json object
/**
 * @brief Appent <tr> row to table
 * @param {Object} item 
 * @param {Number} index 
 * @returns HtmlElement row
 */
function AppendTheRule(item, index) {
    const table = document.getElementById('rules_list_table');
    const tBody = table.getElementsByTagName('tbody')[0];
    let new_row = tBody.insertRow(-1);
    index % 2 == 0 ? new_row.classList.add("tr_even") : new_row.classList.add("tr_odd");
    let checkbox_cell = new_row.insertCell(-1);
    let checkbox = document.createElement('input');
    checkbox.classList.add('list_checkbox');
    checkbox.setAttribute('type', 'checkbox');
    checkbox_cell.appendChild(checkbox);
    checkbox_cell.classList.add('padding_td');
    InsertCell(new_row, item.id, 'rule_id');
    InsertCell(new_row, item.perm.device.vid, 'rule_vid');
    InsertCell(new_row, item.perm.device.pid, 'rule_pid');
    InsertCell(new_row, item.perm.device.serial, 'rule_serial');
    InsertCell(new_row, item.perm.users[0].name, "rule_user");
    InsertCell(new_row, item.perm.groups[0].name, "rule_group");
    BindEditableRowKeys(new_row);
    return new_row;
}

/**
 * Append an ampty <tr> row
 */
function AppendRow(item) {
    const table = document.getElementById('rules_list_table');
    const table_body = table.tBodies[0];
    const rows_count = table_body.querySelectorAll('tr').length;
    if (!item) item = CreateEmptyRuleObj();
    let new_row = AppendTheRule(item, rows_count);
    new_row.classList.add('row_appended_by_user');
    RowSelect(table, new_row);
    BindDoubleClick(new_row);
    MakeTheRowEditable(new_row);
    new_row.querySelector('input.list_checkbox').addEventListener('change', OnRowChecked);
}

function CreateRuleTemplate(vid, pid, serial) {
    let obj = CreateEmptyRuleObj();
    if (!vid || !pid || !serial) return obj;
    obj.perm.device.vid = vid;
    obj.perm.device.pid = pid;
    obj.perm.device.serial = serial;
    return obj;
}


function CreateEmptyRuleObj() {
    let device = {
        vid: "",
        pid: "",
        serial: ""
    };
    let user = {
        name: "",
        uid: ""
    };
    let group = {
        name: "",
        gid: ""
    };
    let perms = {
        device: device,
        users: [user],
        groups: [group]
    };
    let item = {
        id: "",
        perm: perms
    }
    return item;
}

/**
 * @brief Insert a <td> cell to the <tr> row
 * @param {HtmlElement} row 
 * @param {string} text 
 * @param {string} htmlclass 
 */
function InsertCell(row, text, htmlclass) {
    let cell = row.insertCell(-1);
    cell.classList.add('padding_td');
    if (htmlclass) cell.classList.add(htmlclass);
    let text_span = document.createElement('span');
    let cell_text = document.createTextNode(text);
    text_span.appendChild(cell_text);
    if (htmlclass) text_span.classList.add(htmlclass + '_val');
    text_span.classList.add('span_val');
    cell.appendChild(text_span);
}

// ------------------ bind events ----------------------

// single select for table rows
function BindRowSelect() {
    let table = document.getElementById('rules_list_table');
    table.addEventListener('click', OnRowClicked);
}

// bind keybord keys to rows, enable if editable
function BindEditableRowKeys(row) {
    row.tabIndex = 0;
    row.addEventListener('keyup', (event) => {
        try{
        let tr = event.target.parentElement.parentElement;
        let curr_index = -1;
        if (tr.classList.contains('editing') && event.keyCode == 13) {
            let inputs = tr.querySelectorAll('.edit_inline');

            inputs.forEach((input, index) => {
                if (input === event.target) curr_index = index;
            });
            // not last input go to next input
            if (curr_index >= 0 && curr_index < inputs.length - 1) {
                inputs[curr_index + 1].focus();
            } else if (curr_index = inputs.length - 1) {
                event.target.dispatchEvent(new Event('focusout'));
            }
        }
        } catch (e){
            console.log(e.message);
            return;
        }

    });
}

function OnRowClicked(event) {
    const clicked_row = event.target.closest('tr');
    let table = document.getElementById('rules_list_table');
    // multiselect checkboxes by shift+click
    if (event.shiftKey) {
        let tr_all = table.tBodies[0].querySelectorAll('tr');
        let tr_selected_old = table.tBodies[0].querySelector('tr.tr_selected');
        let index_selected_old = -1;
        let index_selected_new = -1;
        tr_all.forEach((item, index) => {
            if (item == tr_selected_old) index_selected_old = index;
            if (item == clicked_row) index_selected_new = index;
        });
        if (index_selected_new >= 0 && index_selected_old >= 0) {
            let begin = Math.min(index_selected_new, index_selected_old);
            let end = Math.max(index_selected_new, index_selected_old);
            for (let i = begin; i <= end; ++i) {
                let checkbox = tr_all[i].querySelector('input.list_checkbox')
                checkbox.checked = true;
                checkbox.dispatchEvent(new Event('change', { bubbles: true }));
            }
        }
    }
    // multiselect checkboxes by ctrl+click
    if (event.ctrlKey) {
        let checkbox = clicked_row.querySelector('input.list_checkbox');
        checkbox.checked = true;
        checkbox.dispatchEvent(new Event('change', { bubbles: true }));
    }
    RowSelect(table, clicked_row);
}

function RowSelect(table, row) {
    if (!row || row.parentElement.nodeName != 'TBODY') return;
    const rows = table.tBodies[0].querySelectorAll('tr');
    rows.forEach(row => {
        if (row.classList.contains('tr_selected')) {
            row.classList.remove('tr_selected');
        }
    });
    EnableButton(document.getElementById('reset_rule_btn'));
    row.classList.toggle('tr_selected');
    EnableButton(document.getElementById('delete_curr_btn'));
    if (!row.classList.contains('editing') && !row.classList.contains('tr_deleted_rule'))
        EnableButton(document.getElementById("edit_rule_btn"));
}

// multiselect checkbox in the header of a table
function BindTableHeaderCheckbox(table) {
    if (!table) return;
    try {
        let header_checkbox = table.tHead.querySelector('input.list_checkbox');
        header_checkbox.checked = false;
        header_checkbox.addEventListener('change', function (e) {
            let checkbox = e.target;
            if (!checkbox) return;
            let inputs;
            try {
                table = checkbox.parentElement.parentElement.parentElement.parentElement;
                let body = table.tBodies[0];
                inputs = body.querySelectorAll('input.list_checkbox');

            }
            catch (e) {
                console.log(e.message);
            }
            if (!inputs) return;
            const event = new Event('change');
            inputs.forEach(el => {
                el.checked = checkbox.checked;
                el.dispatchEvent(event);
            });
        });
    }
    catch (e) {
        console.log(e.message);
    }
}


// Row Checkbox in table body
// enable "delete" button if some rows are checked, disable if nothing is "checked"
function BindRowCheckbox() {
    let table = document.getElementById('rules_list_table');
    try {
        let checkboxes = table.tBodies[0].querySelectorAll('input.list_checkbox');
        if (!checkboxes) return;
        checkboxes.forEach(checkbox => {
            checkbox.addEventListener('change', OnRowChecked);
        });
    }
    catch (e) {
        console.log(e.message);
    }
}

// row checked in table body
function OnRowChecked(e) {
    // if checked - enable button delete
    if (e.target.checked == true) {
        let delete_btn = document.getElementById('delete_rule_btn');
        if (delete_btn.disabled == true) EnableButton(delete_btn);
    }
    // if not checked, check all rows, if nothing is checked - disable button
    else if (e.target.checked == false) {
        DisableButtonDeleteIfNoRowsCkecked();
    }
}

// if no row are checked - disable the "delete checked" button
function DisableButtonDeleteIfNoRowsCkecked() {
    try {
        let tbody = document.getElementById('rules_list_table').tBodies[0];
        if (!tbody || tbody.nodeName != "TBODY") return;
        let inputs = tbody.querySelectorAll('input.list_checkbox');
        let some = false;
        for (let i = 0; i < inputs.length; ++i) {
            if (inputs[i].checked) {
                some = true;
                break;
            }
        }
        // if nothing is checked
        if (!some) {
            DisableButton(document.getElementById('delete_rule_btn'));
            // uncheck a checkbox in the header
            document.getElementById('rules_list_table').tHead.querySelector('input.list_checkbox').checked = false;
        }
    }
    catch (e) {
        console.log(e);
        return;
    }
}

/**
 * @brief Disable a button
 * @param {HtmlElement} button 
 */
function DisableButton(button) {
    if (!button) return;
    button.disabled = true;
    button.classList.add('like_btn_disabled');
    button.classList.remove('like_btn');
}

/**
 * @brief Enable a button
 * @param {HtmlElement} button 
 */
function EnableButton(button) {
    if (!button) return;
    button.disabled = false;
    button.classList.remove('like_btn_disabled');
    button.classList.add('like_btn');
}


/** 
 * @brief bind boudle-clicks on table cells
 *  @param {HtmlElement} row 
 */
function BindDoubleClick(row) {
    // double click on user
    let table = document.getElementById('rules_list_table');
    let user_cells;
    let group_cells
    // if no row -bind all rows
    if (!row) {
        user_cells = table.querySelectorAll('td.rule_user');
        group_cells = table.querySelectorAll('td.rule_group');
    }
    else {
        user_cells = row.querySelectorAll('td.rule_user');
        group_cells = row.querySelectorAll('td.rule_group');
    }
    user_cells.forEach(cell => {
        cell.addEventListener('dblclick', function (e) { DblClickOnUserOrGroup(e, 'users_list'); });
    });
    // double click on groups
    group_cells.forEach(cell => {
        cell.addEventListener('dblclick', function (e) { DblClickOnUserOrGroup(e, 'groups_list'); });
    });
    // double click on vid
    let vid_cells = table.querySelectorAll('td.rule_vid');
    vid_cells.forEach(cell => {
        cell.addEventListener('dblclick', DblClickOnEditable)
    });
    // double click on pid
    let pid_cells = table.querySelectorAll('td.rule_pid');
    pid_cells.forEach(cell => {
        cell.addEventListener('dblclick', DblClickOnEditable)
    });
    // double click on serial
    let serial_cells = table.querySelectorAll('td.rule_serial');
    serial_cells.forEach(cell => {
        cell.addEventListener('dblclick', DblClickOnEditable)
    });
}


// "edit_row" button
function BinEditRow() {
    let button = document.getElementById('edit_rule_btn');
    DisableButton(button);
    button.addEventListener('click', function (e) {
        let table = document.getElementById('rules_list_table');
        let selected_row = table.querySelector('tr.tr_selected');
        if (selected_row) {
            // edit the row if not deleted
            if (!selected_row.classList.contains("tr_deleted_rule"))
                MakeTheRowEditable(selected_row);
        }
    });
};


/**
 * @brief Bind Delete checked rows button click event
 */
function BindDeleteCheckedRows() {
    let button = document.getElementById('delete_rule_btn');
    DisableButton(button);
    button.addEventListener('click', function (e) {
        let table = document.getElementById('rules_list_table');
        try {
            let checkboxes = table.tBodies[0].querySelectorAll('input.list_checkbox');
            checkboxes.forEach(checkbox => {
                if (!checkbox.checked) return;
                let tr = checkbox.parentElement.parentElement;
                DeleteRow(tr);
            });
        }
        catch (e) {
            console.log(e.message);
            return;
        }
    });
}

/**
 *  @brief delete one selected row
 */
function DeleteSelectedRow() {
    let table = document.getElementById('rules_list_table');
    try {
        let selected = table.tBodies[0].querySelectorAll('tr.tr_selected');
        if (selected.length == 0) return;
        let tr = selected[0];
        DeleteRow(tr);
    } catch (e) {
        console.log(e);
        return;
    }
}

/**
 * @brief Delete one row
 * @param HtmlElement tr 
 */
function DeleteRow(tr) {
    // if a selected row is going to be deleted, disable the "edit" button
    if (tr.classList.contains('tr_selected')) {
        DisableButton(document.getElementById('edit_rule_btn'));
        DisableButton(document.getElementById('delete_curr_btn'));
    }
    if (tr.classList.contains('row_appended_by_user')) {
        let checkbox = tr.querySelector('input.list_checkbox');
        checkbox.checked = false;
        tr.remove();
    } else {
        ResetRow(tr); // reset if some change were made              
        if (tr && tr.nodeName == "TR") tr.classList.add('tr_deleted_rule'); // mark as deleted
    }
    DisableButtonDeleteIfNoRowsCkecked();
    ShowToolTipIfSomeInvalid();
}


// ------------------- local data ---------------------

// Save lists of possible users and groups to the local storage
function SetUsersAndGroups(data) {
    try {
        let json_obj = JSON.parse(data);
        localStorage.setItem("users_list", JSON.stringify(json_obj.users));
        localStorage.setItem("groups_list", JSON.stringify(json_obj.groups));
    }
    catch (e) {
        console.log(e.message);
    }
}

// ------------------- Reset ---------------------


function BindReset() {
    let button = document.getElementById('reset_rule_btn');
    DisableButton(button);
    // reset a row
    button.addEventListener('click', function (e) {
        let table = document.getElementById('rules_list_table');
        let selected_row = table.querySelector('tr.tr_selected');
        if (selected_row) {
            ResetRow(selected_row);
            ShowSaveButtonIfSomethigChaged();
        }
    });
    // reset all
    let button_all = document.getElementById('reset_all_btn');
    button_all.addEventListener('click', function (e) {
        ResetRulesList();
        DisableButton(document.getElementById('edit_rule_btn'));
        DisableButton(document.getElementById('delete_curr_btn'));
        DisableButton(document.getElementById('reset_rule_btn'));
        DisableButtonDeleteIfNoRowsCkecked();
        DisableButton(document.getElementById('btn_save'));
    });

}

// ---------------------- Select user or group -----------------------------

function DblClickOnUserOrGroup(event, storage_name) {
    let td_el;
    let span_el;
    if (event.target.nodeName == "TD") {
        td_el = event.target;
        span_el = td_el.querySelector('span.span_val');
    } else if (event.target.nodeName == "SPAN") {
        span_el = event.target;
        td_el = span_el.parentElement;
    }
    if (td_el.parentElement.classList.contains('tr_deleted_rule')) return;
    let target_width = td_el.offsetWidth + 'px';
    if (span_el)
        span_el.classList.add('hidden');
    if (td_el) {
        let dropdown = CreateUserGroupSelect(storage_name);
        dropdown.style.width = target_width;
        BindRemoveSelectOnFocusLost(dropdown);
        td_el.classList.toggle('padding_td');
        td_el.appendChild(dropdown);
        td_el.style.width = target_width;
        dropdown.focus();
    }
}

// create a <select> dropdown list
function CreateUserGroupSelect(storage_name) {
    const stored_items = localStorage.getItem(storage_name);
    const items = stored_items ? JSON.parse(stored_items) : [];
    let dropdown = document.createElement("select");
    const empty_option = document.createElement('option');
    empty_option.text = "";
    empty_option.value = "-";
    dropdown.add(empty_option);
    items.forEach(item => {
        const option = document.createElement('option');
        option.text = item.name;
        option.value = item.name;
        dropdown.add(option);
    });
    dropdown.classList.add('edit_inline');
    return dropdown;
}

// bind event on focus out - if nothing was chosen, remove select
function BindRemoveSelectOnFocusLost(select_element) {
    select_element.addEventListener("focusout", (event) => {
        SaveSelectValueToSpan(event.target);
    });
    select_element.addEventListener('keyup', (event) => {
        if (event.keyCode == 27) // escape
            SaveSelectValueToSpan(event.target, true);
        if (event.keyCode == 13) // enter
            SaveSelectValueToSpan(event.target);
    });

}

function SaveSelectValueToSpan(select, do_not_save) {
    if (!select.parentElement) return;
    let sibling_span = select.parentElement.querySelector('span.span_val');
    let parent_td = select.parentElement;
    if (!sibling_span || !parent_td || parent_td.nodeName != 'TD') return;
    let empty_val = !select.value || select.value == '-';
    let sibling_span_empty = sibling_span.textContent.length == 0;
    if (parent_td.parentElement.classList.contains('row_appended_by_user') && empty_val && sibling_span_empty) {
        parent_td.classList.add('td_value_changed');
        parent_td.classList.add('td_value_bad');
    }
    if (!do_not_save && !empty_val && select.value != sibling_span.textContent) {
        sibling_span.textContent = select.value;
        parent_td.classList.add('td_value_changed');
        parent_td.classList.remove('td_value_good');
        parent_td.classList.remove('td_value_bad');
        // validation
        let valid = false;
        if (parent_td.classList.contains('rule_user')) {
            valid = ValidateUser(select.value);
        } else if (parent_td.classList.contains('rule_group')) {
            valid = ValidateGroup(select.value);
        }
        if (valid) {
            parent_td.classList.add('td_value_good');
        } else {
            parent_td.classList.add('td_value_bad');
        }

    }
    parent_td.classList.toggle('padding_td');
    select.remove();
    sibling_span.classList.remove('hidden');
    ShowToolTipIfSomeInvalid();
}

// ------------- edit vid,pid or serial ----------------------

function DblClickOnEditable(event) {
    let td_el;
    let span_el;
    if (event.target.nodeName == "TD") {
        td_el = event.target;
        span_el = td_el.querySelector('span.span_val');
    } else if (event.target.nodeName == "SPAN") {
        span_el = event.target;
        td_el = span_el.parentElement;
    }
    if (td_el) {
        if (td_el.parentElement.classList.contains('tr_deleted_rule')) return;
        let target_width = td_el.offsetWidth + 'px';
        if (span_el) span_el.classList.add('hidden');
        let input = CreateInput(span_el.textContent);
        input.style.width = target_width;
        BindRemoveInputOnFocusLost(input);
        td_el.classList.toggle('padding_td');
        td_el.appendChild(input);
        td_el.style.width = target_width;
        input.focus();
    }
}

// create an <input> text elemnet 
function CreateInput(initial_text) {
    let input = document.createElement("input");
    input.value = initial_text;
    input.classList.add("edit_inline");
    return input;
}

function BindRemoveInputOnFocusLost(input) {
    // add event on focus out - if nothing was chosen, remove select
    input.addEventListener("focusout", (event) => {
        SaveInputValueToSpan(event.target);
    });
    input.addEventListener('keyup', (event) => {
        if (event.keyCode == 27)
            SaveInputValueToSpan(event.target, true);
        if (event.keyCode == 13)
            SaveInputValueToSpan(event.target);
    });

}

function SaveInputValueToSpan(input, do_not_save) {
    let parent_td = input.parentElement;
    if (!parent_td) return;
    let initial_text;
    let sibling_span = input.parentElement.querySelector('span.span_val');
    if (sibling_span) initial_text = sibling_span.textContent;
    if (!sibling_span || initial_text === 'undefined' || !parent_td || parent_td.nodeName != 'TD') return;
    let input_val=input.value.replace(/[&<>"']/g, "");
    if (parent_td.parentElement.classList.contains('row_appended_by_user') && input_val.length == 0) {
        parent_td.classList.add('td_value_changed');
        parent_td.classList.add('td_value_bad');
    }
    if (!do_not_save && input_val != "" && input_val != sibling_span.textContent) {
        sibling_span.textContent = input_val;
        parent_td.classList.add('td_value_changed');
        parent_td.classList.remove('td_value_good');
        parent_td.classList.remove('td_value_bad');
        let valid = false;
        if (parent_td.classList.contains('rule_vid') || parent_td.classList.contains('rule_pid')) {
            valid = ValidateVidPid(input_val);
        }
        else if (parent_td.classList.contains('rule_serial')) {
            valid = input_val.length > 0;
        }
        if (valid) {
            parent_td.classList.add('td_value_good');
        } else {
            parent_td.classList.add('td_value_bad');
        }
    }
    parent_td.classList.toggle('padding_td');
    input.remove();
    sibling_span.classList.remove('hidden');
    ShowToolTipIfSomeInvalid();

}

function ShowToolTipIfSomeInvalid() {
    let table = document.getElementById('rules_list_table');
    let not_valid_cells = table.tBodies[0].querySelectorAll('td.td_value_bad');
    let warinig = document.getElementById('validation_warning');
    if (not_valid_cells.length > 0) {
        warinig.style.display = 'block';
        DisableButton(document.getElementById('btn_save'));
    }
    else {
        warinig.style.display = 'none';
        ShowSaveButtonIfSomethigChaged();     
    }
}

function ShowSaveButtonIfSomethigChaged(){
    let table = document.getElementById('rules_list_table');
    let tr_changed=table.tBodies[0].querySelectorAll('td.td_value_changed');
    let tr_deleted=table.tBodies[0].querySelectorAll('tr.tr_deleted_rule');
    if ((tr_changed && tr_changed.length>0) || (tr_deleted && tr_deleted.length>0) ){
         EnableButton(document.getElementById('btn_save'));
    }
    else{
        DisableButton(document.getElementById('btn_save'));
    }
}



// ------------------- edit row -------------

function ResetRow(row) {
    let td = row.querySelector('td.rule_id');
    if (!td) return;
    row.classList.remove('tr_deleted_rule');
    row.querySelectorAll('td').forEach(cell => {
        cell.classList.remove('td_value_bad');
        cell.classList.remove('td_value_good');
        cell.classList.remove('td_value_changed');
    });
    let val_span = td.querySelector('span.span_val');
    if (!val_span) return;
    let rule_id = val_span.textContent;
    try {
        let rule_obj;
        // if the rule is appended, use an empty object
        if (row.classList.contains('row_appended_by_user')) {
            rule_obj = CreateEmptyRuleObj();
        }
        // else read the rule from the local storage
        else {
            let data = localStorage.getItem("rules_data");
            if (!data) return;
            let rules_arr = JSON.parse(data);
            rule_obj = rules_arr.find(obj => obj.id === rule_id);
        }
        if (!rule_obj) return;
        //vid 
        let vid_span = row.querySelector('td.rule_vid').querySelector('span.span_val');
        if (vid_span) vid_span.textContent = rule_obj.perm.device.vid;
        let pid_span = row.querySelector('td.rule_pid').querySelector('span.span_val');
        if (pid_span) pid_span.textContent = rule_obj.perm.device.pid;
        let serial_span = row.querySelector('td.rule_serial').querySelector('span.span_val');
        if (serial_span) serial_span.textContent = rule_obj.perm.device.serial;
        let user_span = row.querySelector('td.rule_user').querySelector('span.span_val');
        if (user_span) user_span.textContent = rule_obj.perm.users[0].name;
        let group_span = row.querySelector('td.rule_group').querySelector('span.span_val');
        if (group_span) group_span.textContent = rule_obj.perm.groups[0].name;
        let td_elements = row.querySelectorAll('td');
        td_elements.forEach(td => {
            td.classList.remove('td_value_changed');
        });
        ShowToolTipIfSomeInvalid();
    }
    catch (e) {
        console.log(e.message);
    }
}

// make the whole row editable
function MakeTheRowEditable(row) {
    if (row.classList.contains('editing')) return;
    row.classList.add('editing');
    const td_elements = row.querySelectorAll('td');
    td_elements.forEach(td => {
        let td_class;
        if (td.classList.contains('rule_vid')) td_class = 'rule_vid';
        else if (td.classList.contains('rule_pid')) td_class = 'rule_pid';
        else if (td.classList.contains('rule_serial')) td_class = 'rule_serial';
        else if (td.classList.contains('rule_user')) td_class = 'rule_user';
        else if (td.classList.contains('rule_group')) td_class = 'rule_group';
        // create inputs
        const input_classes = ['rule_vid', 'rule_pid', 'rule_serial'];
        let is_input = input_classes.some(class_name => td.classList.contains(class_name));
        let span_el = td.querySelector('span.span_val');
        let target_width = td.offsetWidth + 'px';
        if (is_input && span_el) {
            let input = CreateInput(span_el.textContent);
            BindRowFocusLost(input);
            input.style.width = target_width;
            td.classList.toggle('padding_td');
            span_el.classList.toggle('hidden');
            td.appendChild(input);
            td.style.width = target_width;
            // focus on first field
            if (td_class === 'rule_vid') input.focus();
        }
        // create selects
        const select_classes = ['rule_user', 'rule_group'];
        let is_select = select_classes.some(class_name => td.classList.contains(class_name));
        if (is_select && span_el) {
            let storage;
            if (td_class == "rule_user") storage = 'users_list';
            else if (td_class == "rule_group") storage = 'groups_list';
            if (storage) {
                let select = CreateUserGroupSelect(storage);
                BindRowFocusLost(select);
                select.style.width = target_width;
                td.classList.toggle('padding_td');
                span_el.classList.toggle('hidden');
                td.appendChild(select);
                td.style.width = target_width;
            }
        }
    });
    DisableButton(document.getElementById("edit_rule_btn"));
}

// if all inputs in row lost their focus, save values, remove inputs
function BindRowFocusLost(input) {
    input.addEventListener("focusout", (event) => {
        let row_has_focus = false;
        let row = event.target.parentElement.parentElement;
        if (row.nodeName != 'TR') return;
        let td_elements = row.querySelectorAll('td');
        td_elements.forEach(td => {
            let input = td.querySelector('input');
            let select = td.querySelector('select');
            if (input && input == event.relatedTarget) row_has_focus = true;
            if (select && select == event.relatedTarget) row_has_focus = true;
        });
        // if row has no focus -save values and remove inputs and selects
        if (row_has_focus) return;
        td_elements.forEach(td => {
            let curr_input = td.querySelector('input');
            let curr_select = td.querySelector('select');
            if (!curr_input && !curr_select) return;
            if (curr_input)
                SaveInputValueToSpan(curr_input);
            else if (curr_select)
                SaveSelectValueToSpan(curr_select);
        });
        row.classList.remove('editing');
        EnableButton(document.getElementById("edit_rule_btn"));
    });
    input.addEventListener('keyup', (event) => {
        if (event.keyCode == 27) {
            let row = event.target.parentElement.parentElement;
            if (row.nodeName != 'TR') return;
            let td_elements = row.querySelectorAll('td');
            td_elements.forEach(td => {
                let curr_input = td.querySelector('input');
                let curr_select = td.querySelector('select');
                if (!curr_input && !curr_select) return;
                if (curr_input)
                    SaveInputValueToSpan(curr_input, true);
                else if (curr_select)
                    SaveSelectValueToSpan(curr_select, true);
            });
            row.classList.remove('editing');
            EnableButton(document.getElementById("edit_rule_btn"));
        }
    });
}

// ------------------- Validators -------------

function ValidateVidPid(str) {
    // Check if the string is a valid hexadecimal number
    const hexRegex = /^[0-9A-Fa-f]{1,4}$/;
    if (!hexRegex.test(str)) {
        return false;
    }
    // Check if the hexadecimal number can be represented in two bytes
    const hexValue = parseInt(str, 16);
    return hexValue >= 0 && hexValue <= 0xFFFF && str.length == 4;
}

function ValidateUser(user) {
    let us = user.trim();
    let arr = JSON.parse(localStorage.getItem('groups_list'));
    let some = arr.some(el => el.name == us);
    return some;
}

function ValidateGroup(group) {
    let us = group.trim();
    let arr = JSON.parse(localStorage.getItem('groups_list'));
    let some = arr.some(el => el.name == us);
    return some;
}

// ------------------- Save -------------

// Save -send to the backend
function SaveRules(){
    let tbody=document.getElementById('rules_list_table').tBodies[0];
    let rows=tbody.querySelectorAll('tr');
    let res ={
        created:[],
        updated:[],
        deleted:[],
        notchanged:[]
    }
    rows.forEach(row=>{
        let rule_id=row.querySelector('td.rule_id').textContent.trim();
        let is_existing=!row.classList.contains('row_appended_by_user') && rule_id.length>0;
        let is_deleted=is_existing && row.classList.contains('tr_deleted_rule');
        let changed_vals=row.querySelectorAll('td.td_value_changed');
        let is_updated=is_existing && !is_deleted && changed_vals && changed_vals.length>0;
        let is_not_changed=is_existing && !is_updated && !is_deleted; 
        let is_created=!is_existing && changed_vals && changed_vals.length>0;
        let vid,pid,serial,user,group,all_fields;
        try{
            vid=row.querySelector('span.rule_vid_val').textContent.trim();
            pid=row.querySelector('span.rule_pid_val').textContent.trim();
            serial=row.querySelector('span.rule_serial_val').textContent.trim();
            user=row.querySelector('span.rule_user_val').textContent.trim();
            group=row.querySelector('span.rule_group_val').textContent.trim();
            all_fields=vid && pid && serial && user && group;
        } catch (e){
            console.log(e.message);
            return;
        }
        // rules is created
        if (is_created  && all_fields){
            res.created.push({
                id:rule_id,
                vid:vid,
                pid:pid,
                serial:serial,
                user:user,
                group:group
            });
        }
        // rule deleted
        if (is_deleted){
            res.deleted.push(rule_id);
            return;
        }
        // updated
        if (is_updated && all_fields){
            let vid_updated=row.querySelector('td.rule_vid').classList.contains('td_value_changed');
            let pid_updated=row.querySelector('td.rule_pid').classList.contains('td_value_changed');
            let serial_updated=row.querySelector('td.rule_serial').classList.contains('td_value_changed');
            let user_updated=row.querySelector('td.rule_user').classList.contains('td_value_changed');
            let group_updated=row.querySelector('td.rule_group').classList.contains('td_value_changed');
            res.updated.push({
                id:rule_id,
                vid: vid_updated ? vid : "",
                pid: pid_updated ? pid : "",
                serial: serial_updated ? serial :"",
                user:user_updated ? user : "",
                group: group_updated ? group : ""
            });
            return;
        }
        if (is_not_changed){
            res.notchanged.push(rule_id);
            return;
        }
    });
    // save to hidden input and fire an event for lisp
    document.getElementById('hidden_inp_data_to_save').value=JSON.stringify(res);
    document.getElementById('btn_save').dispatchEvent(new Event('rules_data_ready'));
}

