import BaseStore from './BaseStore.mjs';

const db = new BaseStore({
    host:'192.168.1.204',
    user: 'root',
    password: '123456',
    database: 'ormtest'
});

const user = [
    [ 'id', 'integer', { primary: true }],
    [ 'username', String ],
    [ 'password', String ]
]

async function main()
{
    try
    {
        await db.ready();
        const User = await db.Model('User', user);
        await User.createTable();
        await User.create({
            username: '+1s',
            password: '+1s'
        });
        await User.create({
            username: '+1s',
            password: '+1s'
        });
        await User.create({
            username: '-1s',
            password: '-1s'
        });
        const secrets = await User.where({
            password: '+1s'
        }).all();
        console.log(secrets);
    }
    catch(e)
    {
        console.log(e);
    }
}

main();