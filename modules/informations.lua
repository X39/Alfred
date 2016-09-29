alfred.registerCmd(function (content, receiver, sender, kind)
	alfred.respond("I am a bot. You can see my src at https://github.com/X39/Alfred/", receiver, sender);
end, "who are you", "", false, false);

alfred.registerCmd(function (content, receiver, sender, kind)
	local chnlist = alfred.getChannelList();
	local str = "You can find me at ";
	for k,v in pairs(chnlist) do
		str = str .. v .. " ";
	end
	alfred.respond(str, receiver, sender);
end, "where are you", "", false, false);