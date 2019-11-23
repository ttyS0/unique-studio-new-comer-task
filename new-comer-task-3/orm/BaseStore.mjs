import BaseModel from './BaseModel.mjs';
import knex from 'knex';
import mysql from 'mysql2/promise.js';

export default class BaseStore
{
    conn = null;
    config = {}
    models = {}
    constructor(props)
    {
        this.config = props || {};
        // this.conn = knex(this.config)
        // this.ready();
    }
    async ready()
    {
        this.conn = await mysql.createConnection(this.config);
    }
    Model(name, model)
    {
        if(typeof(model) == 'function')
        {
            //
        }
        else if(typeof(model) == 'object')
        {
            this.models[name] = new BaseModel(this, name.toLowerCase(), model);
        }
        return this.models[name];
    }
}