# RCF 关于[战斗流程协议.md](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/docs/%E6%88%98%E6%96%97%E6%B5%81%E7%A8%8B%E5%8D%8F%E8%AE%AE.md) 一些改动建议

仅从 json schema 设计上讲，有一些个人认为可以调整的地方
提出来讨论一下，欢迎逐行 comment

由于不太清楚数据消费方是否有任何使用限制，如有问题也请多多包涵以及指正

基于以下方案的 OpenApi schema 可供参考
https://towa.stoplight.io/docs/maacopilot/3d8359fceacd0-operations

### Stage
- 增加 stage 
  - 以 object 形式记录目标 stage 方便拓展
  - stage: `{ name, id, code }`
  - 拆分 id 以精确匹配
  - 拆分 code 方便编写
  - 优先级: id > code > name
- 弃用 stage_name
  - 多义单一字段从使用上讲容易出现问题

### Groups
- opers -> operators
  - 保持整体命名风格一致性
  - 可以整合到 

- 【建议】统一使用 Group 类型数据，弃用 operators
  - 改名为 Roles 来表示一组可替换干员的概念
  - 如无可替代，数组只填入一个干员即可
  - roles: `[{ name, alternatives: [ Operator ] }]`

- operator
  - 增加 id
    - 准确定位干员，提高兼容性
    - 优先 name 匹配
  - skill_usage 使用 enum 增加可读性及可维护性
    - 使用数字并不能减少手写难度，因为需要查表或者脑内维护 mapping
    - 可以考虑：
      - 0 不自动使用 -> disabled
      - 1 好了就用 -> loop
        - 话说这个需要 action 配套嘛？
      - 2 仅使用一次 -> once
      - 3 自动判断使用时机 -> smart / auto （随意，反正是饼
  - requirements
    - potentiality -> potential_rank
      - 对齐用语

### Operators
- operators
  - 标识为 deprecated

### Actions
- 拆分不同类型 action，使用 oneOf 以准确描述 schema
- rear_delay -> post_delay
  - …… No comments

- 【建议】将字段归入几个大类
  - type
    - 增加 关闭技能 / StopSkill
  - condition: `{ kills } / { cost_changes } / { time } ...`
    - 如果需要复杂条件可以拓展成 `{ and: [ { kill }] } / { or: [ { kill }, { enemey_shown: { selector } } ] }` 等等
  - pre_delay
  - post_delay
  - selector: { name } / { location }
  - data: { type } / { direction }
    - 仅描述动作使用数据
  - override: { skill_usage }
    - 包括所有需要修改配置的数据
  - comment


### Miscellaneous
- 建议 doc -> comment / desc / description
  - 用词问题， document 指代的概念范围太大了
- 增加 version
  - 提高向后兼容性
  - 方便增加新字段时处理数据迁移