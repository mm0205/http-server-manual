-- リクエストを処理する関数
function request_handler(request)
    response_text = "これはLuaのスクリプトです。\r\n"

    for key, val in pairs(request) do
        response_text = response_text .. "key=" .. key .. ", value=" .. val .. "\r\n"
    end
    return response_text
end

function test()
    test_table = {}
    test_table["key1"] = "日本語"
    test_table["key2"] = "6"
    return request_handler(test_table)
end