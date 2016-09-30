alfred.registerCmd(function(c, recv, send, kind, arg1)
    local isRunning = alfred.getChannelVar(recv, "mondspiel", "running");
        print(tostring(isRunning));
    if(isRunning == "true") then
        alfred.respond("Game already in progress ...", recv, send);
    else
        alfred.respond("Lets start!", recv, send);
        alfred.setChannelVar(recv, "mondspiel", "running", "true");
        alfred.setChannelVar(recv, "mondspiel", "current", "0");
        alfred.setChannelVar(recv, "mondspiel", "attempts", "0");
        alfred.setChannelVar(recv, "mondspiel", "lastsolver", send);
    end
end, "mondspiel", "", false, false);
alfred.registerMsg(function(c, recv, send)
    local isRunning = alfred.getChannelVar(recv, "mondspiel", "running");
    if(isRunning == "true") then
        local currentExpected = tonumber(alfred.getChannelVar(recv, "mondspiel", "current"));
        local attempts = tonumber(alfred.getChannelVar(recv, "mondspiel", "attempts"));
        local lastsolver = alfred.getChannelVar(recv, "mondspiel", "lastsolver");
        
        if(lastsolver == send) then
            alfred.setChannelVar(recv, "mondspiel", "attempts", tostring(attempts + 1));
            alfred.respond("+1 attempt for trying to progress on your own.", recv, send);
        elseif(currentExpected == 0) then
            if(c ~= ". ") then
                alfred.setChannelVar(recv, "mondspiel", "attempts", tostring(attempts + 1));
            else
                alfred.setChannelVar(recv, "mondspiel", "current", tostring(currentExpected + 1));
                alfred.setChannelVar(recv, "mondspiel", "lastsolver", send);
                alfred.respond("dot", recv, send);
            end
        elseif(currentExpected == 1) then
            if(c ~= ". ") then
                alfred.setChannelVar(recv, "mondspiel", "attempts", tostring(attempts + 1));
            else
                alfred.setChannelVar(recv, "mondspiel", "current", tostring(currentExpected + 1));
                alfred.setChannelVar(recv, "mondspiel", "lastsolver", send);
                alfred.respond("dot dot", recv, send);
            end
        elseif(currentExpected == 2) then
            if(c ~= ", ") then
                alfred.setChannelVar(recv, "mondspiel", "attempts", tostring(attempts + 1));
            else
                alfred.setChannelVar(recv, "mondspiel", "current", tostring(currentExpected + 1));
                alfred.setChannelVar(recv, "mondspiel", "lastsolver", send);
                alfred.respond("dot dot comma", recv, send);
            end
        elseif(currentExpected == 3) then
            if(c ~= "- ") then
                alfred.setChannelVar(recv, "mondspiel", "attempts", tostring(attempts + 1))
            else
                alfred.setChannelVar(recv, "mondspiel", "running", "false");
                alfred.respond("dot dot comma dash, required " .. tostring(attempts) .. " attempts to finish.", recv, send);
            end
        end
    end
end);
