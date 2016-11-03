alfred.registerCmd(function (content, receiver, sender, channel, msg)
    	alfred.sendPrivMsg(msg, channel);
end, "tell", "channel;msg;", true, true);