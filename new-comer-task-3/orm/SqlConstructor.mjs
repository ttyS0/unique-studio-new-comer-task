export default class SqlConstructor
{
    static ormTypes = {
        'integer': 'int', 
        'bigInteger': 'bigint',
        'string': 'varchar',
        'decimal': 'decimal'
    }
    static defaultSize = {
        varchar: 255
    }
    static ormTypeToSqlType(type)
    {
        let t = type.match(/[^\(]*/);
        let s = type.match(/\((\d+)\)/);
        t = t && t[0] && this.ormTypes[t[0]];
        s = s ? s[1] * 1 : this.defaultSize[t];
        return s ? `${t}(${s})` : t;
    }
    static valueToAttribute(attr, value)
    {
        if(attr.type.startsWith('integer') || attr.type.startsWith('bigInteger'))
        {
            return value;
        }
        else
        {
            return `'` + value + `'`;
        }
    }
    static tableExists(db, name)
    {
        return `SELECT * FROM information_schema.TABLES WHERE TABLE_SCHEMA = '${db}' AND TABLE_NAME = '${name}'`;
    }
    static createTable(name, attributes)
    {
        let attr_queries = [];
        let primary = [];
        let index = [];
        let unique = [];
        for(let k in attributes)
        {
            let attr = attributes[k];
            let attr_query = [];
            attr_query.push('`' + attr.key + '`');
            attr_query.push(this.ormTypeToSqlType(attr.type));
            attr.null || attr_query.push('NOT NULL');
            attr.default && attr_query.push('DEFAULT ' + this.valueToAttribute(attr, attr.default));
            attr.primary && attr_query.push('AUTO_INCREMENT');
            attr_queries.push(attr_query.join(' '));
            attr.primary && primary.push('`' + attr.key + '`');
            attr.index && index.push('`' + attr.key + '`');
            attr.unique && unique.push('`' + attr.key + '`');
        }
        primary.length && attr_queries.push(`PRIMARY KEY (${primary.join(', ')})`);
        index.length && attr_queries.push(`INDEX (${index.join(', ')})`);
        unique.length && attr_queries.push(`UNIQUE (${unique.join(', ')})`);
        return `CREATE TABLE \`${name}\` (${attr_queries.join(', ')});`
    }
    static preparedArguments(args)
    {
        let fields = [], values = [];
        for(let k in args)
        {
            let v = args[k];
            fields.push(k);
            values.push(v);
        }
        return { fields, values }
    }
    static commaPrepare(fields)
    {
        fields.map((v, k) => fields[k] = '`' + fields[k] + '`=?');
        return fields.join(', ');
    }
    static eqPrepare(fields)
    {
        fields.map((v, k) => fields[k] = '`' + fields[k] + '`=?');
        return fields.join(' AND ');
    }
    static insert(name, fields)
    {
        let placeholders = [];
        fields.map((v, k) => fields[k] = '`' + fields[k] + '`');
        for(let k in fields)
        {
            placeholders.push('?');
        }
        return `INSERT INTO \`${name}\` (${fields.join(', ')}) VALUES (${placeholders.join(', ')})`;
    }
    static update(name, whereFields, updateFields)
    {
        return `UPDATE \`${name}\` SET ${this.commaPrepare(updateFields)} WHERE ${this.eqPrepare(whereFields)};`;
    }
    static select(name, whereFields, selectFields)
    {
        selectFields && selectFields.map((v, k) => selectFields[k] = '`' + selectFields[k] + '`');
        return `SELECT ${selectFields ? selectFields.join(',') : '*' } FROM \`${name}\`${whereFields ? (' WHERE ' + this.eqPrepare(whereFields)) : ''}`
    }
    static delete(name, whereFields)
    {
        return `DELETE FROM ${name} WHERE ${this.eqPrepare(whereFields)}`;
    }
}