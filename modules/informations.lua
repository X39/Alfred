alfred.registerCmd(function (content, receiver, sender, kind)
	alfred.respond("I am a bot. You can see my src at https://github.com/X39/Alfred/", receiver, sender);
end, "who are you", "", false, false);