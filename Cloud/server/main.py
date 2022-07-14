from flask import Blueprint, render_template, request
from flask_login import login_required, current_user
from .models import User
from . import db

main = Blueprint('main', __name__)

@main.route('/')
def index():
    return render_template('index.html')

@main.route('/profile')
@login_required
def profile():
    return render_template('profile.html', name=current_user.name, tel_num=current_user.tel_num)

@main.route('/profile', methods=['POST'])
@login_required
def profile_post():
    tel_num = request.form.get('tel_num')
    current_user.tel_num = tel_num
    db.session.commit()
    return render_template('profile.html', name=current_user.name, tel_num=current_user.tel_num)