import inflection from 'inflection';
import SqlConstructor from './SqlConstructor.mjs'
import Utils from './Utils.mjs'
export default class BaseModel
{
    parentStore = null;
    name = null;
    attributes = [];
    primary = null;
    _table_name = null;
    _where = null;
    _key = null;
    _jsTypeToKnex(type)
    {
        if(type == String)
        {
            return 'string(255)';
        }
        else if(type == BigInt)
        {
            return 'bigInteger';
        }
        else if(type == Number)
        {
            return 'decimal';
        }
        else
        {
            return type;
        }
    }
    _parseModel(model)
    {
        // this.primary = model[0] ?( typeof(model[0] == 'string') ? model[0] : model[0][0]) : null;
        for(let k in model)
        {
            let attr = {};
            let m = model[k];
            if(typeof(m) == 'string')
            {
                attr.key = m;
                attr.type = 'string';
                attr.primary = false;
                attr.index = false;
                attr.default = null;
            }
            else
            {
                attr.key = m[0];
                attr.type = m[1] ? this._jsTypeToKnex(m[1]) : this._jsTypeToKnex('string');
                attr.primary = m[2] ? ((m[2].primary && (this.primary = m[0])) || false) : false;
                attr.index = m[2] ? (m[2].index || false) : false;
                attr.default = m[2] ? (m[2].default || null) : null;
            }
            this.attributes.push(attr);
        }
    }
    constructor(parentStore, name, model)
    {
        this.parentStore = parentStore;
        this.name = name;
        this._parseModel(model);
        this._table_name = inflection.underscore(inflection.pluralize(this.name))
    }
    attribute(name, options)
    {
        for(let k in this.attributes)
        {
            if(this.attributes[k].key == name && options)
            {
                this.attributes[k] = {
                    ...this.attributes[k],
                    ...options
                }
            }
        }
    }
    async _exists()
    {
        const [ row ] = await this.parentStore.conn.execute(SqlConstructor.tableExists(this.parentStore.config.database, this._table_name));
        return row.length != 0;
    }
    async createTable()
    {
        const exist = await this._exists();
        if(exist)
        {
            throw new Error('Table has existed.');
        }
        else
        {
            return await this.parentStore.conn.execute(SqlConstructor.createTable(this._table_name, this.attributes));
        }
    }
    async create(data)
    {
        if(!await this._exists())
        {
            throw new Error('Table doesn\'t exist.');
        }
        let final = {};
        for(let k in this.attributes)
        {
            let attr = this.attributes[k];
            if(data[attr.key])
            {
                final[attr.key] = (data[attr.key]);
            }
        }
        const { fields, values } = SqlConstructor.preparedArguments(final);
        const [ row ] = await this.parentStore.conn.execute(SqlConstructor.insert(this._table_name, fields), values);
        return await this.find(row.insertId);
    }
    async find(key)
    {
        const { fields, values } = SqlConstructor.preparedArguments({ [this.primary]: key });
        const [ row ] = await this.parentStore.conn.execute(SqlConstructor.select(this._table_name, fields), values);
        return row && row[0];
    }
    // Under where: first
    async first()
    {
        if(this._where != null)
        {
            const { fields, values } = SqlConstructor.preparedArguments(this._where);
            const [ row ] = await this.parentStore.conn.execute(SqlConstructor.select(this._table_name, fields), values);
            return row && row[0];
        }
    }
    // Under where: all
    async all()
    {
        if(this._where != null)
        {
            const { fields, values } = SqlConstructor.preparedArguments(this._where);
            const [ row ] = await this.parentStore.conn.execute(SqlConstructor.select(this._table_name, fields), values);
            return row;
        }
    }
    // Under where: remove all
    async remove()
    {
        if(this._where != null)
        {
            const { fields, values } = SqlConstructor.preparedArguments(this._where);
            const [ row ] = await this.parentStore.conn.execute(SqlConstructor.delete(this._table_name, fields), values);
            return row;
        }
    }
    where(condition)
    {
        let copy = Utils.clone(this);
        copy._where = condition;
        return copy;
    }
    async update(data)
    {
        // const w = SqlConstructor.preparedArguments({ [this.primary]: data[this.primary] });
        // delete(data[this.primary]);
        // const n = SqlConstructor.preparedArguments(data);
        // const v = n.values.concat(w.values);
        // return await this.parentStore.conn.execute(SqlConstructor.update(this._table_name, w.fields, n.fields), v);
        const key = data[this.primary];
        delete(data[this.primary]);
        let q = [];
        for(let k in data)
        {
            q.push(k + `='${data[k]}'`);
        }
        //console.log(`UPDATE ${this._table_name} SET ${q.join(', ')} WHERE ${this.primary}='${key}'`);
        return await this.parentStore.conn.execute(`UPDATE ${this._table_name} SET ${q.join(', ')} WHERE ${this.primary}='${key}'`);
    }
    // Delete obj
    async delete(data)
    {
        const { fields, values } = SqlConstructor.preparedArguments({ [this.primary]: data[this.primary] });
        const [ row ] = await this.parentStore.conn.execute(SqlConstructor.delete(this._table_name, fields), values);
        return row;
    }
    // Delete key(s)
    async deleteAll(keys)
    {
        for(let k of keys)
        {
            const { fields, values } = SqlConstructor.preparedArguments({ [this.primary]: k });
            const [ row ] = await this.parentStore.conn.execute(SqlConstructor.delete(this._table_name, fields), values);
        }
    }
}