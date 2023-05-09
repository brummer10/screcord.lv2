function (event) {

    var position = "0";

    function handle_event (symbol, value) {
        switch (symbol) {
            case 'CLIP':
                position = (Math.min(Math.floor(value), 1) * 64).toFixed(1);
                event.icon.find ('[mod-role=CLIP]').css({backgroundPosition: "-"+position+"px 0px"});
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
