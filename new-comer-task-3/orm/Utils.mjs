export default class Utils 
{
    static clone(o) {
        var F = function () {};
        F.prototype = o;
        return new F();
    };
}
