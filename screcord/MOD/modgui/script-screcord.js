function (event) {

    var position = "0";

    function log_meter (db) {
        var def = 0.000001; /* Meter deflection %age */

        if (db < -70.0) {
            def = 0.0000001;
        } else if (db < -60.0) {
            def = (db + 70.0) * 0.25;
        } else if (db < -50.0) {
            def = (db + 60.0) * 0.5 + 2.5;
        } else if (db < -40.0) {
            def = (db + 50.0) * 0.75 + 7.5;
        } else if (db < -30.0) {
            def = (db + 40.0) * 1.5 + 15.0;
        } else if (db < -20.0) {
            def = (db + 30.0) * 2.0 + 30.0;
        } else if (db < 6.0) {
            def = (db + 20.0) * 2.5 + 50.0;
        } else {
            def = 115.0;
        }

        return (Math.floor((def/115.0) * 63) * 10).toFixed(1);
    }

    function handle_event (symbol, value) {
        switch (symbol) {
            case 'CLIP':
                position = (Math.min(Math.floor(value), 1) * 64).toFixed(1);
                event.icon.find ('[mod-role=CLIP]').css({backgroundPosition: `-${position}px 0px`});
                break;
            case 'LMETER':
                position = log_meter (value);
                event.icon.find ('[mod-role=LMETER]').css({backgroundPosition: `-${position}px 0px`});
                break;
            case 'RMETER':
                position = log_meter (value);
                event.icon.find ('[mod-role=RMETER]').css({backgroundPosition: `-${position}px 0px`});
                break;
            case 'REC':
                if (value > 0) {
                    event.icon.find ('[mod-role=TAPE]').css({'animation-play-state': 'running'});
                    event.icon.find ('[mod-role=TAPE2]').css({'animation-play-state': 'running'});
                } else {
                    event.icon.find ('[mod-role=TAPE]').css({'animation-play-state': 'paused'});
                    event.icon.find ('[mod-role=TAPE2]').css({'animation-play-state': 'paused'});
                }
                
                break;
            default:
                break;
        }
    }

    if (event.type == 'start') {
        var ports = event.ports;
        for (var p in ports) {
            handle_event (ports[p].symbol, ports[p].value);
        }
    }
    else if (event.type == 'change') {
        handle_event (event.symbol, event.value);
    }
}
