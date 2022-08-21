// 全局JSON数据
// 必填字段可以放在这里，预设空值
const data = {
    actions: [],
};

// 模板
const input_text = () => {
    return $('<input type="text" class="form-control">');
};

const input_oper_name = () => {
    return $('<input type="text" class="form-control" placeholder="干员名字" list="oper_names">');
}

const skill_usage = () => {
    return $('<select id="skill_usage" class="form-control" style="width:auto">' +
        '<option value=0>不自动使用</option>' +
        '<option value=1>好了就用，有多少次用多少次</option>' +
        '<option value=2>好了就用，仅使用一次</option>' +
        '<option value=3>自动判断使用时机</option>' +
        '</select>');
};

const move_up_icon = (data, arr) => {
    return $('<button type="button" class="btn btn-primary"><span class="ui-icon ui-icon-triangle-1-n"></span></button>')
        .click(() => {
            const index = arr.indexOf(data);
            if (index > 0) {
                arr.splice(index, 1);
                arr.splice(index - 1, 0, data);
                loadData();
            }
        });
}

const move_down_icon = (data, arr) => {
    return $('<button type="button" class="btn btn-primary"><span class="ui-icon ui-icon-triangle-1-s"></span></button>')
        .click(() => {
            const index = arr.indexOf(data);
            if (index < arr.length - 1) {
                arr.splice(index, 1);
                arr.splice(index + 1, 0, data);
                loadData();
            }
        });
}

const delete_icon = (data, arr) => {
    return $('<button type="button" class="btn btn-danger"><span class="ui-icon ui-icon-closethick"></span></button>')
        .click(() => {
            arr.splice(arr.indexOf(data), 1);
            loadData();
        });
};

const action_type = () => {
    return $('<select id="type" class="form-control" style="width:auto">' +
        '<option value="二倍速">二倍速</option>' +
        '<option value="部署">部署</option>' +
        '<option value="技能">技能</option>' +
        '<option value="撤退">撤退</option>' +
        '<option value="子弹时间">子弹时间</option>' +
        '<option value="技能用法">技能用法</option>' +
        '<option value="摆完挂机">摆完挂机</option>' +
        '<option value="打印">打印</option>' +
        '</select>');
};

const direction = () => {
    return $('<select id="direction" class="form-control" style="width:auto">' +
        '<option value=""></option>' +
        '<option value="上">上</option>' +
        '<option value="下">下</option>' +
        '<option value="左">左</option>' +
        '<option value="右">右</option>' +
        '</select>');
};

const oper = (oper_data, arr) => {
    const tr = $('<tr>');
    // 干员名字
    const name_input = input_oper_name()
        .val(oper_data.name ?? "")
        .change(function () { oper_data.name = $(this).val(); });
    tr.append($('<td>').append(name_input));
    // 技能
    const skill_input = input_text()
        .attr('type', 'number')
        .val(String(oper_data.skill ?? 0))
        .change(function () { oper_data.skill = Number($(this).val()); });
    tr.append($('<td>').append(skill_input));
    // 技能用法
    const skill_usage_input = skill_usage()
        .val(String(oper_data.skill_usage ?? 0))
        .change(function () { oper_data.skill_usage = Number($(this).val()); });
    tr.append($('<td>').append(skill_usage_input));
    // 移动删除
    tr.append($('<td style="white-space:nowrap;">')
        .append(move_up_icon(oper_data, arr))
        .append(move_down_icon(oper_data, arr))
        .append(delete_icon(oper_data, arr)));
    return tr;
}

const action = (action_data, arr) => {
    const tr = $('<tr>');
    // 类别
    const type_input = action_type()
        .val(action_data.type ?? "")
        .change(function () { action_data.type = $(this).val(); });
    tr.append($('<td>').append(type_input));
    // 击杀数
    const kills_input = input_text()
        .attr('type', 'number')
        .val(String(action_data.kills ?? ""))
        .change(function () { action_data.kills = $(this).val() !== "" ? Number($(this).val()) : undefined; });
    tr.append($('<td>').append(kills_input));
    // 费用变化
    const cost_changes = input_text()
        .attr('type', 'number')
        .val(String(action_data.cost_changes ?? ""))
        .change(function () { action_data.cost_changes = $(this).val() !== "" ? Number($(this).val()) : undefined; });
    tr.append($('<td>').append(cost_changes));
    // 干员
    const name_input = input_oper_name()
        .val(action_data.name ?? "")
        .change(function () { action_data.name = $(this).val(); });
    tr.append($('<td>').append(name_input));
    // x坐标
    const location_x_input = input_text()
        .attr('type', 'number')
        .val(String((action_data.location ?? [0, 0])[0] ?? ""))
        .change(function () {
            action_data.location = action_data.location ?? [0, 0];
            action_data.location[0] = $(this).val() !== "" ? Number($(this).val()) : 0;
        });
    tr.append($('<td>').append(location_x_input));
    // y坐标
    const location_y_input = input_text()
        .attr('type', 'number')
        .val(String((action_data.location ?? [0, 0])[1] ?? ""))
        .change(function () {
            action_data.location = action_data.location ?? [0, 0];
            action_data.location[1] = $(this).val() !== "" ? Number($(this).val()) : 0;
        });
    tr.append($('<td>').append(location_y_input));
    // 方向
    const direction_input = direction()
        .val(action_data.direction ?? "")
        .change(function () { action_data.direction = $(this).val(); });
    tr.append($('<td>').append(direction_input));
    // 前延迟
    const pre_delay_input = input_text()
        .attr('type', 'number')
        .val(String(action_data.pre_delay ?? ""))
        .change(function () { action_data.pre_delay = $(this).val() !== "" ? Number($(this).val()) : undefined; });
    tr.append($('<td>').append(pre_delay_input));
    // 后延迟
    const rear_delay_input = input_text()
        .attr('type', 'number')
        .val(String(action_data.rear_delay ?? ""))
        .change(function () { action_data.rear_delay = $(this).val() !== "" ? Number($(this).val()) : undefined; });
    tr.append($('<td>').append(rear_delay_input));
    // 文本
    const doc_input = input_text()
        .val(action_data.doc ?? "")
        .change(function () { action_data.doc = $(this).val(); });
    tr.append($('<td>').append(doc_input));
    // 颜色
    const doc_color_input = input_text()
        .val(action_data.doc_color ?? "")
        .change(function () { action_data.doc_color = $(this).val(); });
    tr.append($('<td>').append(doc_color_input));
    // 移动删除
    tr.append($('<td style="white-space:nowrap;">')
        .append(move_up_icon(action_data, arr))
        .append(move_down_icon(action_data, arr))
        .append(delete_icon(action_data, arr)));
    return tr;
};

const group = (group_data, arr) => {
    // 群组名
    const name_div_row = $('<div class="row">');
    const name_div_col = $('<div class="col-10">');
    const name_input = input_text()
        .val(group_data.name ?? "")
        .change(function () { group_data.name = $(this).val(); });
    name_div_col.append(name_input);
    const move_delete_col = $('<div class="col-2">');
    move_delete_col.append(move_up_icon(group_data, arr))
        .append(move_down_icon(group_data, arr))
        .append(delete_icon(group_data, arr));
    name_div_row.append(name_div_col).append(move_delete_col);

    // 群组干员列表
    const table = $('<table class="table table-bordered table-condensed table-striped">' +
        '<thead>' +
        '<tr>' +
        '<th>干员名字</th>' +
        '<th>技能</th>' +
        '<th>技能用法</th>' +
        '<th></th>' +
        '</tr>' +
        '</thead>' +
        '<tbody></tbody>' +
        '</table>');
    (group_data.opers ?? []).forEach(oper_data => table.find('tbody').append(oper(oper_data, () => {
        group_data.opers = group_data.opers.filter(o => o !== oper_data);
        loadData();
    })));

    const new_oper_button = $('<button class="btn btn-primary">新增干员</button>')
        .click(() => {
            group_data.opers = group_data.opers ?? [];
            group_data.opers.push({});
            loadData();
        });

    return $('<div>')
        .append(name_div_row)
        .append(table)
        .append(new_oper_button)
        .append('<br><br>');
};

// 加载数据
const loadData = () => {
    $('#stage_name').val(data.stage_name ?? "");
    $('#details').val((data.doc ?? {}).details ?? "");
    $('#title').val((data.doc ?? {}).title ?? "");
    // TODO: requirements

    $('#opers tbody').html('');
    (data.opers ?? []).forEach(oper_data => $('#opers tbody').append(
        oper(oper_data, data.opers)
    ));

    $('#groups').html('');
    (data.groups ?? []).forEach(group_data => $('#groups').append(
        group(group_data, data.groups)
    ));

    $('#actions tbody').html('');
    (data.actions ?? []).forEach(action_data => $('#actions tbody').append(action(action_data, data.actions)));
};

$(document).ready(() => {
    // 绑定事件
    $('#stage_name').change(() => data.stage_name = $('#stage_name').val());
    $('#details').change(() => {
        data.doc = data.doc ?? {};
        data.doc.details = $('#details').val();
    });
    $('#title').change(() => {
        data.doc = data.doc ?? {};
        data.doc.title = $('#title').val();
    });

    $('#opers_new').click(() => {
        data.opers = data.opers ?? [];
        data.opers.push({});
        loadData();
    });

    $('#actions_new').click(() => {
        data.actions = data.actions ?? [];
        data.actions.push({});
        loadData();
    });

    $('#groups_new').click(() => {
        data.groups = data.groups ?? [];
        data.groups.push({});
        loadData();
    })

    $('#download_json').click(() => {
        const result = JSON.stringify(data, null, 4);
        const file = new Blob([result], { type: 'application/json' });
        const fileName = data.stage_name + '_' + (data.opers?.map(o => o.name).join('+') ?? '') + '.json';
        const url = window.URL.createObjectURL(file);
        const link = document.createElement('a');
        link.href = url;
        link.setAttribute('download', fileName);
        link.click();
    });

    $('#upload_json').click(() => $('#json_file').click());

    $('#json_file').change(function (e) {
        const files = e.target.files;
        const f = files[0];
        const reader = new FileReader();
        reader.onload = (() => (e) => {
            Object.assign(data, JSON.parse(e.target.result));
            loadData();
        })(f);
        reader.readAsText(f);
    });

    // 干员名字自动填充
    charName.sort();
    charName.forEach(c => $('#oper_names').append($('<option>').text(c)));

    loadData();
});
