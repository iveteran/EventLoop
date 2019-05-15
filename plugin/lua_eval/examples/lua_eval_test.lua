
function print_parameters(params)
  print("parameters length: " .. string.len(params))
  print("parameters value: " .. params)
end

function print_table(arg_table)
  print("table length: " .. #arg_table)
  print("table kv: ")
  for k, v in pairs(arg_table) do
    print(k .. ": " .. v)
  end
end

function example_binary(udata)
  print("[example_binary] arg data: ")
  print_parameters(udata)
  return os.date() .. " -> " .. udata
end

function example_array(arg_table)
  print("[example_array] arg table: ")
  print_table(arg_table)
  ret_table = {}
  ret_table[1] = 'example_array'
  ret_table[2] = os.date()
  return ret_table
end

function example_map(arg_table)
  print("[example_map] arg table: ")
  print_table(arg_table)
  ret_table = {}
  -- NOTE: table's key MUST is string
  ret_table['1'] = 'example_map'
  ret_table['2'] = os.date()
  ret_table['foo'] = 'aaaa'
  ret_table['bar'] = 'bbbb'
  return ret_table
end

print("---------- unit test ----------")
result = example_binary("unit test")
print("[example_binary] result: " )
print(result)
print("-------------------------------")
test_table = {'foo', 'bar'}
result = example_array(test_table)
print("[example_array] result: ")
print_table(result)
print("-------------------------------")
test_table = {['aa'] = 'foo', ['bb'] = 'bar'}
result = example_map(test_table)
print("[example_map] result: ")
print_table(result)
print("-------- end test -------------")
