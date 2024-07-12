test_pipeline_a = {
    "sub": ["A_sub", "A_sub2"],
    "next": ["A_next"],
    "onErrorNext": ["A_onErrorNext"],
    "exceededNext": ["A_exceededNext"],
    "reduceOtherTimes": ["A_reduceOtherTimes"],
}

test_pipeline_b = {
    "sub": ["B_sub", "B_sub2"],
    "next": ["B_next"],
    "onErrorNext": ["B_onErrorNext"],
    "exceededNext": ["B_exceededNext"],
    "reduceOtherTimes": ["B_reduceOtherTimes"],
}

test_info_a = {
    "algorithm": "MatchTemplate",
    "action": "ClickSelf",
    "subErrorIgnored": False,
    "maxTimes": 1,
    "preDelay": 0,
    "postDelay": 0,
    "retryTimes": 0,
    "roi": [],
    "rectMove": [],
    "cache": False,
    "specificRect": [100, 100, 50, 50],
    "specialParams": [],
}

test_match_template_a = {
    "algorithm": "MatchTemplate",
    "template": "A.png",
    "templThreshold": 0.8,
    "maskRange": [1, 255],
}

test_virtual_task = {
    "sub": ["#virtual_sub", "#virtual_sub2"],
    "next": ["#virtual_next"],
    "onErrorNext": ["#virtual_onErrorNext"],
    "exceededNext": ["#virtual_exceededNext"],
}
