
function print_parameters(params)
  print("parameters: " .. params)
  print("parameters length: " .. string.len(params))
end

function enter(udata)
  print_parameters(udata)
  return os.date() .. " -> hello " .. udata
end

result = enter("test")
print(result)
