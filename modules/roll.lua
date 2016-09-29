alfred.registerCmd(function (content, receiver, sender, kind, from, to)
	local numFrom = tonumber(from);
	local numTo = tonumber(to);
	if numFrom == nil or numTo == nil or numFrom > numTo then
    	alfred.respond(alfred.getRandomResponse("error"), receiver, sender);
	else
    	alfred.respond(tostring(math.random(from, to)), receiver, sender);
	end
end, "roll", "from=0;to=30000;", false, false);