declare namespace XianNet {
    function debug(message: String): void;
    function info(message: String): void;
    function warning(message: String): void;
    function error(message: String): void;
    
    /**
     * @param serviceName 服务名（路径不带拓展名）
     * @returns 返回serviceId
     */
    function newService(serviceName: String): Number;

    function call(serviceId: Number, functionName: String, message: any): any;
    function send(serviceId: Number, functionName: String, message: any): any;
}

