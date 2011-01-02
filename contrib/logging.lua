--for logging
logdir = "/absolute/path/to/gtkabber-logs/"

function message_cb (from, to, msg)
        --logging
        who = nil
        if string.find(from, jid) then
                who = to
        else
                who = from
        end
        who = string.sub(who, 0, string.find(who, "/") - 1)
        fh, asd = io.open(logdir .. who, "a")
        if fh == nil then
                gtkabber.print("Unable to open logfile: " .. asd .. "\n")
        else
                fh:write(os.date("%d/%m/%Y %H:%M:%S ") .. from .. ": " .. msg .. "\n")
                fh:close()
        end
end
